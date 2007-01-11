/**
 * @author Samuel Larkin
 * @file blanklayout.cc  Layout for logger that just prints the logevent without any artifacts.
 *
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada
 */

#ifndef NO_LOGGING
#include "blanklayout.h"
#include <log4cxx/spi/loggingevent.h>

using namespace Portage::Logging;
using namespace log4cxx;
using namespace log4cxx::spi;

IMPLEMENT_LOG4CXX_OBJECT(BlankLayout)

void BlankLayout::format(ostream& output, const spi::LoggingEventPtr& event) const
{
   output << event->getRenderedMessage() << std::endl;
}
#endif // NO_LOGGING
