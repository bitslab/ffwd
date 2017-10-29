/* A Bison parser, made by GNU Bison 2.4.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006,
   2009, 2010 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     COMMENT = 258,
     END = 259,
     ERROR = 260,
     RESET = 261,
     PARSER_DEBUG = 262,
     INCLUDE = 263,
     CONFIGURE_FILE = 264,
     EMPTY_LINE = 265,
     SERVER = 266,
     SOCKET = 267,
     SERVERS = 268,
     SERVERS_OPTION = 269,
     UNKNOWN_OPTION = 270,
     UNKNOWN = 271,
     BINARY_PROTOCOL = 272,
     BUFFER_REQUESTS = 273,
     CONNECT_TIMEOUT = 274,
     DISTRIBUTION = 275,
     HASH = 276,
     HASH_WITH_NAMESPACE = 277,
     IO_BYTES_WATERMARK = 278,
     IO_KEY_PREFETCH = 279,
     IO_MSG_WATERMARK = 280,
     KETAMA_HASH = 281,
     KETAMA_WEIGHTED = 282,
     NOREPLY = 283,
     NUMBER_OF_REPLICAS = 284,
     POLL_TIMEOUT = 285,
     RANDOMIZE_REPLICA_READ = 286,
     RCV_TIMEOUT = 287,
     REMOVE_FAILED_SERVERS = 288,
     RETRY_TIMEOUT = 289,
     SND_TIMEOUT = 290,
     SOCKET_RECV_SIZE = 291,
     SOCKET_SEND_SIZE = 292,
     SORT_HOSTS = 293,
     SUPPORT_CAS = 294,
     USER_DATA = 295,
     USE_UDP = 296,
     VERIFY_KEY = 297,
     _TCP_KEEPALIVE = 298,
     _TCP_KEEPIDLE = 299,
     _TCP_NODELAY = 300,
     NAMESPACE = 301,
     POOL_MIN = 302,
     POOL_MAX = 303,
     MD5 = 304,
     CRC = 305,
     FNV1_64 = 306,
     FNV1A_64 = 307,
     FNV1_32 = 308,
     FNV1A_32 = 309,
     HSIEH = 310,
     MURMUR = 311,
     JENKINS = 312,
     CONSISTENT = 313,
     MODULA = 314,
     RANDOM = 315,
     TRUE = 316,
     FALSE = 317,
     FLOAT = 318,
     NUMBER = 319,
     PORT = 320,
     WEIGHT_START = 321,
     IPADDRESS = 322,
     HOSTNAME = 323,
     STRING = 324,
     QUOTED_STRING = 325,
     FILE_PATH = 326
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif




