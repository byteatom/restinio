#include <iostream>

#include <restinio/all.hpp>

#include "nlohmann.hpp"

using Json = nlohmann::json;

int main(int argc, char *argv[])
{
	auto router = std::make_unique<restinio::router::express_router_t<>>();

	router->http_post("/files", []( auto req, auto paras){
		std::cout << "size:" << req->body().size() << std::endl;
		if (req->body().empty())
			req->create_response()
				.append_header(restinio::http_field::content_type, "application/json; charset=utf-8")
				.set_body(R"({"result":true})")
				.done();
		return restinio::request_accepted();
	}).notify_body_piece();

	router->http_get("/", []( auto req, auto ){
		req->create_response()
				.append_header(restinio::http_field::content_type, "text/html; charset=utf-8")
				.set_body(R"(<!DOCTYPE html>
						  <html>
							<head>
							  <title>File upload example</title>
							</head>
							<body>
								<form method="post" action="/files" enctype="multipart/form-data">
								 <input type="file" name="file" accept="image/*" multiple>
								 <input type="submit" value="Upload">
								</form>
							</body>
						  </html>)")
				.done();
		return restinio::request_accepted();
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
