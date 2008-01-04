/**
 * @author Samuel Larkin
 * @file blanklayout.h  Layout for logger that just prints the logevent without any artifacts.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef _PORTAGE_BLANK_LAYOUT_H
#define _PORTAGE_BLANK_LAYOUT_H

#ifndef NO_LOGGING
#include <log4cxx/layout.h>

namespace Portage
{
   namespace Logging
   {
      using namespace log4cxx;

      class BlankLayout;
      typedef helpers::ObjectPtrT<BlankLayout> BlankLayoutPtr;

      /// Outputs only the Logging Event without adding anything.
      class LOG4CXX_EXPORT BlankLayout : public Layout
      {
         public:
            DECLARE_LOG4CXX_OBJECT(BlankLayout)
            BEGIN_LOG4CXX_CAST_MAP()
               LOG4CXX_CAST_ENTRY(BlankLayout)
               LOG4CXX_CAST_ENTRY_CHAIN(Layout)
            END_LOG4CXX_CAST_MAP()

            /**
             * Formats the Logging Event in the output.
             * @param output output stream where the logging event is to be displayed
             * @param event the logging event to be formatted to output
             */
            virtual void format(log4cxx::ostream& output, const spi::LoggingEventPtr& event) const;

            /**
             * Always returns true since BlankLayout doesn't handle the throwable object contained within LoggingEvent.
             * @return Always true
             */
            bool ignoresThrowable() const { return true; }

            /// Must be declare but nothing to do.
            virtual void activateOptions() {}

            /// Must be declare but nothing to do.
            virtual void setOption(const String& option, const String& value) {}
      };
   } // namespace Logging
} // namespace Portage

#endif // NO_LOGGING
#endif //_PORTAGE_BLANK_LAYOUT_H
