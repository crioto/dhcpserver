#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>

#include "args.h"
#include "options.h"

void usage(char *msg, int exit_status)
{
    fprintf(exit_status == 0 ? stdout : stderr,
	    "%s", USAGE_TXT);
    
    if (msg) {
	fprintf(exit_status == 0 ? stdout : stderr,
		"\n%s\n", msg);
    }
    
    exit(exit_status);
}
 
void parse_args(int argc, char *argv[], address_pool *pool)
{
    int c;

    opterr = 0;
    // When port is set to 0 dhcpserver will pick port number with getservbyname()
    pool->port = 0;

    while ((c = getopt (argc, argv, "a:d:o:p:s:")) != -1)
	switch (c) {

	case 'a': // parse IP address pool
	    {
		char *opt    = strdup(optarg);
		char *sfirst = opt;
		char *slast  = strchr(opt, ',');
	    
		if (slast == NULL)
		    usage("error: comma not present in option -a.", 1);
		*slast = '\0';
		slast++;
	    
		uint32_t *first, *last;
		
		if (parse_ip(sfirst, (void **)&first) != 4)
		    usage("error: invalid first ip in address pool.", 1);
		
		if (parse_ip(slast, (void **)&last) != 4)
		    usage("error: invalid last ip in address pool.", 1);

		pool->indexes.first   = *first;
		pool->indexes.last    = *last;
		pool->indexes.current = *first;
		
		free(first);
		free(last);
		free(opt);
		
		break;
	    }

	case 'd': // network device to use
	    {
		strncpy(pool->device, optarg, sizeof(pool->device));
		break;
	    }
	    
	case 'o': // parse dhcp option
	    {
		uint8_t id;

		char *opt   = strdup(optarg);
		char *name  = opt;
		char *value = strchr(opt, ',');
		
		if (value == NULL)
		    usage("error: comma not present in option -o.", 1);
		*value = '\0';
		value++;
		
		dhcp_option *option = calloc(1, sizeof(*option));
		
		if((id = parse_option(option, name, value)) == 0) {
		    char msg[128];
		    snprintf(msg, sizeof(msg),
			     "error: invalid dhcp option specified: %s,%s",
			     name, value);
		    usage(msg, 1);
		}
		
		append_option(&pool->options, option);

		if(option->id == IP_ADDRESS_LEASE_TIME)
		    pool->lease_time = ntohl(*((uint32_t *)option->data));

		free(option);
		free(opt);
		break;
	    }

	case 't': // parse pending time
	    {
		time_t *t;

		if(parse_long(optarg, (void **)&t) != 4)
		    usage("error: invalid pending time.", 1);

		pool->pending_time = *t;
		free(t);
		break;
	    }

    case 'p':
        {
            uint32_t *port = 0;
            if (parse_long(optarg, (void **)&port) != 4)
                usage("error: invalid port.", 1);

            if (*port < 1 || *port > 65535)
                usage("error: unvalid port number calue", 1);

            pool->port = *port;
            printf("Port number %d\n", pool->port);
            free(port);
            break;
        }

	case 's': // static binding
	    {
		char *opt = strdup(optarg);
		char *shw  = opt;
		char *sip  = strchr(opt, ',');
		
		if (sip == NULL)
		    usage("error: comma not present in option -s.", 1);
		*sip = '\0';
		    sip++;
		
		uint32_t *ip;
		uint8_t  *hw;
		
		if (parse_mac(shw, (void **)&hw) != 6)
		    usage("error: invalid mac address in static binding.", 1);
		
		if (parse_ip(sip, (void **)&ip) != 4)
		    usage("error: invalid ip in static binding.", 1);
		
		add_binding(&pool->bindings, *ip, hw, 6, 1);
		
		free(ip);
		free(hw);
		free(opt);
	    }
	    
	case '?':
	    usage(NULL, 1);

	default:
	    usage(NULL, 1);
	}

    if(optind >= argc)
	usage("error: server address not provided.", 1);

    uint32_t *ip;

    if (parse_ip(argv[optind], (void **)&ip) != 4)
	usage("error: invalid server address.", 1);

    pool->server_id = *ip;
    
    free(ip);
}
