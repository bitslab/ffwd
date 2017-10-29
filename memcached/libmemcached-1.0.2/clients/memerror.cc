/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary:
 *
 */
#include "config.h"

#include <inttypes.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <unistd.h>

#include <libmemcached/memcached.h>

#include "utilities.h"

#define PROGRAM_NAME "memerror"
#define PROGRAM_DESCRIPTION "Translate a memcached errror code into a string."


/* Prototypes */
void options_parse(int argc, char *argv[]);

static int opt_verbose= 0;

int main(int argc, char *argv[])
{
  options_parse(argc, argv);

  if (argc != 2)
  {
    return EXIT_FAILURE;
  }

  unsigned long value= strtoul(argv[1], (char **) NULL, 10);

  if (value < MEMCACHED_MAXIMUM_RETURN)
  {
    std::cout << memcached_strerror(NULL, (memcached_return_t)value) << std::endl;
  }
  else
  {
    std::cerr << memcached_strerror(NULL, MEMCACHED_MAXIMUM_RETURN) << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}


void options_parse(int argc, char *argv[])
{
  static struct option long_options[]=
    {
      {(OPTIONSTRING)"version", no_argument, NULL, OPT_VERSION},
      {(OPTIONSTRING)"help", no_argument, NULL, OPT_HELP},
      {(OPTIONSTRING)"quiet", no_argument, NULL, OPT_QUIET},
      {(OPTIONSTRING)"verbose", no_argument, &opt_verbose, OPT_VERBOSE},
      {(OPTIONSTRING)"debug", no_argument, &opt_verbose, OPT_DEBUG},
      {0, 0, 0, 0},
    };

  bool opt_version= false;
  bool opt_help= false;
  int option_index= 0;
  while (1) 
  {
    int option_rv= getopt_long(argc, argv, "Vhvds:", long_options, &option_index);
    if (option_rv == -1)
    {
      break;
    }

    switch (option_rv)
    {
    case 0:
      break;

    case OPT_VERBOSE: /* --verbose or -v */
      opt_verbose = OPT_VERBOSE;
      break;

    case OPT_DEBUG: /* --debug or -d */
      opt_verbose = OPT_DEBUG;
      break;

    case OPT_VERSION: /* --version or -V */
      opt_version= true;
      break;

    case OPT_HELP: /* --help or -h */
      opt_help= true;
      break;

    case OPT_QUIET:
      close_stdio();
      break;

    case '?':
      /* getopt_long already printed an error message. */
      exit(EXIT_FAILURE);

    default:
      abort();
    }
  }

  if (opt_version)
  {
    version_command(PROGRAM_NAME);
    exit(EXIT_SUCCESS);
  }

  if (opt_help)
  {
    help_command(PROGRAM_NAME, PROGRAM_DESCRIPTION, long_options, NULL);
    exit(EXIT_SUCCESS);
  }
}
