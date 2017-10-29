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


#include <libtest/common.h>

#include <csignal>

#include <libtest/signal.h>

using namespace libtest;

#define MAGIC_MEMORY 123569

bool SignalThread::is_shutdown()
{
  bool ret;
  pthread_mutex_lock(&shutdown_mutex);
  ret= bool(__shutdown != SHUTDOWN_RUNNING);
  pthread_mutex_unlock(&shutdown_mutex);

  return ret;
}

void SignalThread::set_shutdown(shutdown_t arg)
{
  pthread_mutex_lock(&shutdown_mutex);
  __shutdown= arg;
  pthread_mutex_unlock(&shutdown_mutex);

  if (arg == SHUTDOWN_GRACEFUL)
  {
    if (pthread_kill(thread, SIGUSR2) == 0)
    {
      void *retval;
      pthread_join(thread, &retval);
    }
  }
}

shutdown_t SignalThread::get_shutdown()
{
  shutdown_t local;
  pthread_mutex_lock(&shutdown_mutex);
  local= __shutdown;
  pthread_mutex_unlock(&shutdown_mutex);

  return local;
}

void SignalThread::post()
{
  sem_post(&lock);
}

void SignalThread::test()
{
  assert(magic_memory == MAGIC_MEMORY);
  if (not getenv("LIBTEST_IN_GDB"))
  {
    assert(sigismember(&set, SIGABRT));
    assert(sigismember(&set, SIGQUIT));
    assert(sigismember(&set, SIGINT));
  }
  assert(sigismember(&set, SIGUSR2));
}

SignalThread::~SignalThread()
{
  if (not is_shutdown())
  {
    set_shutdown(SHUTDOWN_GRACEFUL);
  }

#if 0
  if (pthread_equal(thread, pthread_self()) != 0 and (pthread_kill(thread, 0) == ESRCH) == true)
  {
    void *retval;
    pthread_join(thread, &retval);
  }
#endif
  sem_destroy(&lock);
}

extern "C" {

static void *sig_thread(void *arg)
{   
  SignalThread *context= (SignalThread*)arg;

  context->test();
  context->post();

  while (context->get_shutdown() == SHUTDOWN_RUNNING)
  {
    int sig;

    if (context->wait(sig) == -1)
    {
      Error << "sigwait() returned errno:" << strerror(errno);
      continue;
    }

    switch (sig)
    {
    case SIGABRT:
    case SIGUSR2:
    case SIGINT:
    case SIGQUIT:
      if (context->is_shutdown() == false)
      {
        context->set_shutdown(SHUTDOWN_FORCED);
      }
      break;

    default:
      Error << "Signal handling thread got unexpected signal " <<  strsignal(sig);
      break;
    }
  }

  return NULL;
}

}

SignalThread::SignalThread() :
  magic_memory(MAGIC_MEMORY),
  thread(pthread_self())
{
  pthread_mutex_init(&shutdown_mutex, NULL);
  sigemptyset(&set);
  if (not getenv("LIBTEST_IN_GDB"))
  {
    sigaddset(&set, SIGABRT);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGINT);
  }

  sigaddset(&set, SIGUSR2);

  sem_init(&lock, 0, 0);
}


bool SignalThread::setup()
{
  set_shutdown(SHUTDOWN_RUNNING);

  sigset_t old_set;
  sigemptyset(&old_set);
  pthread_sigmask(SIG_BLOCK, NULL, &old_set);

  if (sigismember(&old_set, SIGQUIT))
  {
    Error << strsignal(SIGQUIT) << " has been previously set.";
  }
  if (sigismember(&old_set, SIGINT))
  {
    Error << strsignal(SIGINT) << " has been previously set.";
  }
  if (sigismember(&old_set, SIGUSR2))
  {
    Error << strsignal(SIGUSR2) << " has been previously set.";
  }

  int error;
  if ((error= pthread_sigmask(SIG_BLOCK, &set, NULL)) != 0)
  {
    Error << "pthread_sigmask() died during pthread_sigmask(" << strerror(error) << ")";
    return false;
  }

  if ((error= pthread_create(&thread, NULL, &sig_thread, this)) != 0)
  {
    Error << "pthread_create() died during pthread_create(" << strerror(error) << ")";
    return false;
  }

  sem_wait(&lock);

  return true;
}
