#include <iostream>

#include <restinio/all.hpp>

#include "nlohmann.hpp"

using Json = nlohmann::json;

int main(int argc, char *argv[])
{
	auto router = std::make_unique<restinio::router::express_router_t<>>();

	router->http_get("/", []( auto req, auto ){
		req->create_response()
				.append_header(restinio::http_field::content_type, "text/plain; charset=utf-8")
				.set_body("Hello world")
				.done();
		return restinio::request_accepted();
	});

	router->http_post("/files/absent", []( auto req, auto ){
		std::cout << req->header().request_target() << "->" << req->body() << std::endl;
		req->create_response()
				.append_header(restinio::http_field::content_type, "application/json; charset=utf-8")
				.set_body(req->body())
				.done();
		return restinio::request_accepted();
	});

	router->http_post("/files", []( auto req, auto ){
		std::cout << req->header().request_target() << "->size:" << req->body().size() << ":" << req->body() << std::endl;
		req->create_response()
				.append_header(restinio::http_field::content_type, "application/json; charset=utf-8")
				.set_body(R"({"result":true})")
				.done();
		return restinio::request_accepted();
	});

	router->http_put("/clips", []( auto req, auto ){
		std::cout << req->header().request_target() << "->" << req->body() << std::endl;
		req->create_response()
				.append_header(restinio::http_field::content_type, "application/json; charset=utf-8")
				.set_body(R"({"result":true})")
				.done();
		return restinio::request_accepted();
	});

	router->non_matched_request_handler([](auto req){
		std::cout << req->header().request_target() << "->" << req->body() << std::endl;
		return req->create_response(404, "Not found").connection_close().done();
	});

	struct my_server_traits : public restinio::default_single_thread_traits_t {
			using request_handler_t = restinio::router::express_router_t<>;
	};

	restinio::run(restinio::on_this_thread<my_server_traits>()
					.address("localhost")
					.port(8088)
					.request_handler(std::move(router)));

	return 0;
}
