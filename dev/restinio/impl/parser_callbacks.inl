/*
	restinio
*/

/*!
	Callbacks used with http parser.
*/

template <typename Traits>
inline int
restinio_url_cb( http_parser * parser, const char * at, size_t length )
{
	try
	{
		auto * ctx =
			reinterpret_cast< restinio::impl::http_parser_ctx_t<Traits> * >(
				parser->data );

		ctx->m_header.append_request_target( at, length );

		ctx->m_header.method( restinio::http_method_from_nodejs( parser->method ) );
		ctx->m_connection->register_new_request();
		ctx->m_connection->update_context_entry();
	}
	catch( const std::exception & )
	{
		return 1;
	}

	return 0;
}

template <typename Traits>
inline int
restinio_header_field_cb( http_parser * parser, const char *at, size_t length )
{
	try
	{
		auto * ctx =
			reinterpret_cast< restinio::impl::http_parser_ctx_t<Traits> * >(
				parser->data );

		if( ctx->m_last_was_value )
		{
			ctx->m_current_field_name.assign( at, length );
			ctx->m_last_was_value = false;
		}
		else
		{
			ctx->m_current_field_name.append( at, length );
		}
	}
	catch( const std::exception & )
	{
		return 1;
	}

	return 0;
}

inline void
append_last_field_accessor( http_header_fields_t & fields, string_view_t value )
{
	fields.append_last_field( value );
}

template <typename Traits>
inline int
restinio_header_value_cb( http_parser * parser, const char *at, size_t length )
{
	try
	{
		auto * ctx =
			reinterpret_cast< restinio::impl::http_parser_ctx_t<Traits> * >( parser->data );

		if( !ctx->m_last_was_value )
		{
			ctx->m_header.set_field(
				std::move( ctx->m_current_field_name ),
				std::string{ at, length } );

			ctx->m_last_was_value = true;
		}
		else
		{
			append_last_field_accessor( ctx->m_header, std::string{ at, length } );
		}
	}
	catch( const std::exception & )
	{
		return 1;
	}

	return 0;
}

template <typename Traits>
inline int
restinio_headers_complete_cb( http_parser * parser )
{
	try
	{
		auto * ctx =
			reinterpret_cast< restinio::impl::http_parser_ctx_t<Traits> * >(
				parser->data );

		if( ULLONG_MAX != parser->content_length &&
			0 < parser->content_length
			&& !(ctx->m_handler_entry && ctx->m_handler_entry->is_notify_body_piece()))
		{
			ctx->m_body.reserve(
				::restinio::utils::impl::uint64_to_size_t( parser->content_length) );
		}
	}
	catch( const std::exception & )
	{
		return 1;
	}

	return 0;
}

template <typename Traits>
inline int
restinio_body_cb( http_parser * parser, const char *at, size_t length )
{
	try
	{
		auto * ctx =
			reinterpret_cast< restinio::impl::http_parser_ctx_t<Traits> * >(
				parser->data );
		if (ctx->m_handler_entry && ctx->m_handler_entry->is_notify_body_piece())
			ctx->m_body.assign(at, length);
		else
			ctx->m_body.append(at, length);
	}
	catch( const std::exception & )
	{
		return 1;
	}

	return 0;
}

template <typename Traits>
inline int
restinio_message_complete_cb( http_parser * parser )
{
	// If entire http-message consumed, we need to stop parser.
	http_parser_pause( parser, 1 );

	auto * ctx =
		reinterpret_cast< restinio::impl::http_parser_ctx_t<Traits> * >(
			parser->data );

	ctx->m_message_complete = true;

	if( 0 == parser->upgrade )
		ctx->m_header.should_keep_alive( 0 != http_should_keep_alive( parser ) );
	else
		ctx->m_header.connection( http_connection_header_t::upgrade );

	return 0;
}
