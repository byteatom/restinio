 /*
	Simple example using sendfile facility.
*/

#include <iostream>
#include <random>

#include <restinio/all.hpp>
#include <restinio/transforms/zlib.hpp>

#include <clara/clara.hpp>
#include <fmt/format.h>

//
// app_args_t
//

//
// app_args_t
//

struct app_args_t
{
	bool m_help{ false };
	std::string m_address{ "localhost" };
	std::uint16_t m_port{ 8080 };
	std::size_t m_pool_size{ 1 };
	bool m_trace_server{ false };

	static app_args_t
	parse( int argc, const char * argv[] )
	{
		using namespace clara;

		app_args_t result;

		auto cli =
			Opt( result.m_address, "address" )
					["-a"]["--address"]
					( fmt::format( "address to listen (default: {})", result.m_address ) )
			| Opt( result.m_port, "port" )
					["-p"]["--port"]
					( fmt::format( "port to listen (default: {})", result.m_port ) )
			| Opt( result.m_pool_size, "thread-pool size" )
					[ "-n" ][ "--thread-pool-size" ]
					( fmt::format(
						"The size of a thread pool to run server (default: {})",
						result.m_pool_size ) )
			| Opt( result.m_trace_server )
					[ "-t" ][ "--trace" ]
					( "Enable trace server" )
			| Help(result.m_help);

		auto parse_result = cli.parse( Args(argc, argv) );
		if( !parse_result )
		{
			throw std::runtime_error{
				fmt::format(
					"Invalid command-line arguments: {}",
					parse_result.errorMessage() ) };
		}

		if( result.m_help )
		{
			std::cout << cli << std::endl;
		}

		return result;
	}
};

std::string get_random_nums_str( std::size_t count )
{
	std::random_device r;
	std::mt19937 gen{ r() };
	std::uniform_int_distribution< int > uniform_dist;

	std::ostringstream sout;

	while( 0 != count-- )
	{
		sout << uniform_dist( gen ) << "\r\n";
	}

	return sout.str();
}

using router_t = restinio::router::express_router_t<>;

namespace rtz = restinio::transforms::zlib;

auto make_transform_params(
	const restinio::request_t & req,
	const restinio::query_string_params_t & qp )
{
	const auto accept_encoding =
		req.header().get_field(
			restinio::http_field::accept_encoding,
			"deflate" );

	auto copression_level = [&]{ return restinio::value_or( qp, "level", -1 ); };

	if( std::string::npos != accept_encoding.find( "deflate" ) )
	{
		return rtz::make_deflate_compress_params( copression_level() );
	}
	else if( std::string::npos != accept_encoding.find( "gzip" ) )
	{
		return rtz::make_gzip_compress_params( copression_level() );
	}

	return rtz::make_identity_params();
};

constexpr std::size_t count_threshold = 10000u;

template<typename Resp>
void setup_resp_headers( Resp & resp )
{
	resp
		.append_header( "Server", "RESTinio" )
		.append_header_date_field()
		.append_header( restinio::http_field::content_type, "text/plain; charset=utf-8" );
}

auto make_resp_for_small_count(
	restinio::request_handle_t req,
	const restinio::query_string_params_t & qp,
	std::size_t count )
{
	auto resp = req->create_response();
	setup_resp_headers( resp );

	// Create body appender for a given response
	// (the default one: restinio controlled output).
	auto ba = rtz::body_appender( resp, make_transform_params( *req, qp ) );

	// Set response body data in one step.
	ba.append( get_random_nums_str( count ) );

	// All the data was provided, so finish zlib transformation.
	ba.complete();

	return resp.done();
}

auto make_resp_for_large_count(
	restinio::request_handle_t req,
	const restinio::query_string_params_t & qp,
	std::size_t count )
{
	auto resp = req->create_response< restinio::chunked_output_t >();
	setup_resp_headers( resp );

	// Create body appender for a given response (chunked_output).
	auto ba = rtz::body_appender( resp, make_transform_params( *req, qp ) );

	while( 0 != count )
	{
		// Handle a piece of data.
		const auto current_portion_size = std::min( count, count_threshold );
		ba.make_chunk( get_random_nums_str( current_portion_size ) );
		ba.flush();

		count -= current_portion_size;
	}

	// All the data was provided, so finish zlib transformation.
	ba.complete();

	return resp.done();
}

auto make_router()
{
	auto router = std::make_unique< router_t >();

	router->http_get(
		R"-(/rand/nums)-",
		[ & ]( restinio::request_handle_t req, auto ){

			const auto qp = restinio::parse_query( req->header().query() );
			const std::size_t count = restinio::value_or( qp, "count", 100u );

			if( count < count_threshold )
			{
				// Not to much data to be served in response.
				return make_resp_for_small_count( std::move( req ), qp, count );
			}
			else
			{
				// The data is big enough to use chunked encoding.
				return make_resp_for_large_count( std::move( req ), qp, count );
			}
		} );

	return router;
}

template < typename Server_Traits >
void run_server( const app_args_t & args )
{
	restinio::run(
		restinio::on_thread_pool< Server_Traits >( args.m_pool_size )
			.port( args.m_port )
			.address( args.m_address )
			.concurrent_accepts_count( args.m_pool_size )
			.request_handler( make_router() ) );
}

int main( int argc, const char * argv[] )
{
	try
	{
		const auto args = app_args_t::parse( argc, argv );

		if( !args.m_help )
		{
			if( args.m_trace_server )
			{
				using traits_t =
					restinio::traits_t<
						restinio::asio_timer_manager_t,
						restinio::shared_ostream_logger_t,
						router_t >;

				run_server< traits_t >( args );
			}
			else
			{
				using traits_t =
					restinio::traits_t<
						restinio::asio_timer_manager_t,
						restinio::null_logger_t,
						router_t >;

				run_server< traits_t >( args );
			}
		}
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
