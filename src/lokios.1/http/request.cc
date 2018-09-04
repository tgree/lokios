#include "request.h"

http::request::request():
    state(PARSING_METHOD),
    method(METHOD_UNKNOWN)
{
}

int
http::request::parse(char c)
{
    switch (state)
    {
        case PARSING_METHOD:
            if (c == ' ')
            {
                method = get_method(parsing_key);
                if (method == METHOD_UNKNOWN)
                    throw unrecognized_method_exception();
                state = PARSING_REQUEST_TARGET;
            }
            else if (!kisalnum(c))
                throw header_invalid_exception();
            else
                parsing_key.append(c);
        break;

        case PARSING_REQUEST_TARGET:
            if (c == ' ')
                state = PARSING_HTTP_VERSION_HTTP_SLASH;
            else if (!kisgraph(c))
                throw header_invalid_exception();
            else
                request_target.append(c);
        break;

        case PARSING_HTTP_VERSION_HTTP_SLASH:
            if (c != "HTTP/"[http_slash_pos++])
                throw header_invalid_exception();
            else if (http_slash_pos == 5)
                state = PARSING_HTTP_VERSION_MAJOR;
        break;

        case PARSING_HTTP_VERSION_MAJOR:
            if (!kisdigit(c))
                throw header_invalid_exception();
            version |= ((c - '0') << 4);
            state = PARSING_HTTP_VERSION_DOT;
        break;

        case PARSING_HTTP_VERSION_DOT:
            if (c != '.')
                throw header_invalid_exception();
            state = PARSING_HTTP_VERSION_MINOR;
        break;

        case PARSING_HTTP_VERSION_MINOR:
            if (!kisdigit(c))
                throw header_invalid_exception();
            version |= (c - '0');
            state = PARSING_REQUEST_LINE_CR;
        break;

        case PARSING_REQUEST_LINE_CR:
            if (c != '\r')
                throw header_invalid_exception();
            state = PARSING_REQUEST_LINE_LF;
        break;

        case PARSING_REQUEST_LINE_LF:
            if (c != '\n')
                throw header_invalid_exception();
            state = PARSING_WAIT_NEXT_HEADER;
        break;

        case PARSING_WAIT_NEXT_HEADER:
            if (c == '\r')
                state = PARSING_FINAL_LF;
            else if (!http::istchar(c))
                throw header_invalid_exception();
            else
            {
                parsing_key.clear();
                parsing_key.append(c);
                state = PARSING_HEADER_KEY;
            }
        break;

        case PARSING_HEADER_KEY:
            if (c == ':')
            {
                parsing_val = &headers.emplace(parsing_key,"");
                state = PARSING_HEADER_VAL;
            }
            else if (!http::istchar(c))
                throw header_invalid_exception();
            else
                parsing_key.append(c);
        break;

        case PARSING_HEADER_VAL:
            if (c == '\r')
            {
                while (parsing_val->size() && kisblank((*parsing_val)[-1]))
                    parsing_val->shrink(1);
                state = PARSING_HEADER_LF;
            }
            else if (c == ' ' && parsing_val->size() == 0)
                break;
            else
                parsing_val->append(c);
        break;

        case PARSING_HEADER_LF:
            if (c != '\n')
                throw header_invalid_exception();
            state = PARSING_WAIT_NEXT_HEADER;
        break;

        case PARSING_FINAL_LF:
            if (c != '\n')
                throw header_invalid_exception();
            state = PARSING_DONE;
        break;

        case PARSING_DONE:
            kernel::panic("header parsing already finished");
        break;
    }

    return !(state == PARSING_DONE);
}
