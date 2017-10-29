/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  libtest
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>

#include <cstdlib>
#include <fcntl.h>
#include <getopt.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <libtest/wait.h>

static void version_command(const char *command_name, int major_version, int minor_version)
{
  std::cout << command_name << " " << major_version << "." << minor_version << std::endl;
  exit(EXIT_SUCCESS);
}

static void help_command(const char *command_name,
                         int major_version, int minor_version,
                         const struct option *long_options)
{
  std::cout << command_name << " " << major_version << "." << minor_version << std::endl;
  std::cout << "Current options. A '=' means the option takes a value." << std::endl << std::endl;

  for (uint32_t x= 0; long_options[x].name; x++)
  {
    std::cout << "\t --" << long_options[x].name << char(long_options[x].has_arg ? '=' : ' ') << std::endl;
  }

  std::cout << std::endl;
  exit(EXIT_SUCCESS);
}

static void close_stdio(void)
{
  int fd;
  if ((fd = open("/dev/null", O_RDWR, 0)) < 0)
  {
    return;
  }
  else
  {
    if (dup2(fd, STDIN_FILENO) < 0)
    {
      return;
    }

    if (dup2(fd, STDOUT_FILENO) < 0)
    {
      return;
    }

    if (dup2(fd, STDERR_FILENO) < 0)
    {
      return;
    }

    if (fd > STDERR_FILENO)
    {
      close(fd);
    }
  }
}

enum {
  OPT_HELP,
  OPT_QUIET,
  OPT_VERSION
};

static void options_parse(int argc, char *argv[])
{
  static struct option long_options[]=
  {
    { "version", no_argument, NULL, OPT_VERSION},
    { "help", no_argument, NULL, OPT_HELP},
    { "quiet", no_argument, NULL, OPT_QUIET},
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
    case OPT_HELP: /* --help or -h */
      opt_help= true;
      break;

    case OPT_QUIET:
      close_stdio();
      break;

    case '?':
      /* getopt_long already printed an error message. */
      exit(EXIT_SUCCESS);

    default:
      abort();
    }
  }

  if (opt_version)
  {
    version_command(argv[0], 1, 0);
    exit(EXIT_SUCCESS);
  }

  if (opt_help)
  {
    help_command(argv[0], 1, 0, long_options);
    exit(EXIT_SUCCESS);
  }
}

int main(int argc, char *argv[])
{
  options_parse(argc, argv);

  if (argc == 2)
  {
    libtest::Wait wait(argv[1]);

    if (wait.successful())
      return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
