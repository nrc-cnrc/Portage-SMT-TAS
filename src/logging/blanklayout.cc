/**
 * @author Samuel Larkin
 * @file blanklayout.cc  Layout for logger that just prints the logevent without any artifacts.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
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
