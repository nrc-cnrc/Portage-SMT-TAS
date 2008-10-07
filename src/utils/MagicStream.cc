/**
 * @author Samuel Larkin
 * @file MagicStream.cc  A stream that can be transparently used for cin/cout,
 *                       .txt, .{Z,z,gz} and pipes.
 *
 *
 * COMMENTS: These classes we permit easy integration of a stream that
 *           can be used in the following ways:
 *             - to read from standard in or to write to standard out
 *             - to read/write to a compress gzip file
 *             - to read/write to a plain text file
 *             - to read/write from a pipe
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
               
#include <MagicStream.h>
#include <errors.h>


using namespace Portage;
using namespace std;

// Debug helper :D
// To activate use -DMAGIC_STREAM_DEBUG flag at compile time
inline void log(const string& msg)
{
#ifdef MAGIC_STREAM_DEBUG
   cerr << "\tLOG:\t" << msg << endl;
#endif
}

namespace Portage {
/// Keeps what belongs to MagicStreams together in a clean space.
namespace MagicStream
{
   /// Internal definition of a standard buffer.
   /// When using std_buffer it will automagically close any file descriptor
   /// (int) but will not close file handle (FILE*).
   typedef __gnu_cxx::stdio_filebuf<char> std_buffer;
         
   /// A callable entity that does nothing.  When using the standard outpur or
   /// input, we don't want the MagicStream class to delete cout or cin.
   struct null_deleter {
      /// Pretends to delete the pointer.  Makes the class a callable entity.
      void operator()(void const *) const {
         log("Invoking null deleter");
      }
   };

   /// A callable entity to properly close a pipe.
   struct std_buffer_deleter {
      /// Deletes a std_buffer
      /// @param p buffer to be closed.
      void operator()(std_buffer* p) {
         log("Deleting std_buffer");
         if (p) delete p;
      }
   };

   /// A callable entity to properly close a pipe.
   struct PIPE_deleter {
      FILE* m_pFILE; ///< the file handle to be closed.
      /**
       * Constructor that needs the FILE* that created the pipe.  To properly
       * close the pipe we also need to close its FILE* which was used to
       * created it.
       * @param _p FILE* that created the pipe
       */
      PIPE_deleter(FILE* _p) : m_pFILE(_p) {} 
      /// Closes the pipe.
      /// @param p buffer to be closed.
      void operator()(std_buffer* p) {
         log("Closing PIPE");
         assert(m_pFILE != NULL);
         if (m_pFILE)
            if (pclose(m_pFILE) == -1)
               perror("Warning unable to close pipe properly");
         if (p) delete p;
      }
   };

   /// A callable entity to properly close a file handle.
   struct FILE_HANDLE_deleter {
      FILE* m_fFILE; ///< the file handle to be closed.
      /// Constructor that needs the FILE* to properly close the stream.
      /// @param _f the file handle to be closed.
      FILE_HANDLE_deleter(FILE* _f) : m_fFILE(_f) {}
      /// Closes the file handle.
      /// @param p buffer to be closed.
      void operator()(std_buffer* p) {
         log("Using FILE_HANDLE_deleter");
         assert(p != NULL);
         if (fclose(m_fFILE))
            perror("Warning unable to close file descriptor properly");
         if (p) delete p;
      }
   };

   /// A callable entity to properly close a file descriptor.
   struct FILE_DESCRIPTOR_deleter {
      /// Closes the file descriptor.
      /// @param p buffer to be closed.
      void operator()(std_buffer* p) {
         log("Using FILE_DESCRIPTOR_deleter");
         assert(p != NULL);
         if (p != NULL && p->close())
            perror("Warning unable to close file descriptor properly");
         if (p) delete p;
      }
   };

   /// A callable entity that properly close a file.
   struct close_deleter {
      /// Properly close a file.
      /// @param p file buffer to close
      void operator()(std::filebuf* p) {
         if (p) {
            log("Closing file handle");
            if (!p->close()) error(ETWarn, "Warning Error closing ...");
            delete p;
         }
      }
   };
} // ends namespace MagicStream;
} // ends namespace Portage;
using namespace Portage::MagicStream;


MagicStreamBase::MagicStreamBase(const PipeMode  _p, const OpenMode _b)
: m_pipeMode(_p)
, m_bufferMode(_b)
, Quiet(true)
{}

MagicStreamBase::MagicStreamBase(const PipeMode  _p, const OpenMode _b, int fd, bool closeAtEnd)
: m_pipeMode(_p)
, m_bufferMode(_b)
, Quiet(true)
{
   log("Opening from a file descriptor");
   assert(fd != -1);
   // We need to convert the fd to a FILE* since gcc3.4.3 doesn't have the 
   // same signature when opening from a fd.
   FILE* F = fdopen(fd, pipeMode());
   std_buffer* tmp = new std_buffer(F, bufferMode());
   assert(tmp != NULL);
   assert(tmp->is_open());

   stream = closeAtEnd
      //? stream_type(tmp, FILE_DESCRIPTOR_deleter())
      ? stream_type(tmp, FILE_HANDLE_deleter(F))
      : stream_type(tmp, std_buffer_deleter());
}

MagicStreamBase::MagicStreamBase(const PipeMode  _p, const OpenMode _b, FILE* _f, bool closeAtEnd)
: m_pipeMode(_p)
, m_bufferMode(_b)
, Quiet(true)
{
   log("Opening from a file handle");
   assert(_f != NULL);
   std_buffer* tmp = new std_buffer(_f, bufferMode());
   assert(tmp != NULL);
   assert(tmp->is_open());

   stream = closeAtEnd
      ? stream_type(tmp, FILE_HANDLE_deleter(_f))
      : stream_type(tmp, std_buffer_deleter());
}

MagicStreamBase::~MagicStreamBase()
{
   log("MagicStreamBase::~MagicStreamBase");
}

void MagicStreamBase::close()
{
   log("MagicStreamBase::close");
   stream.reset();
}

bool MagicStreamBase::is_open() const
{
   return stream.use_count() != 0;
}

const MagicStreamBase::PipeMode MagicStreamBase::pipeMode() const
{
   return m_pipeMode;
}

const MagicStreamBase::OpenMode MagicStreamBase::bufferMode() const
{
   return m_bufferMode;
}

// A unified way of opening a pipe
void MagicStreamBase::makePipe(const string& _cmd)
{
   const string cmd(_cmd + (Quiet ? " 2> /dev/null" : ""));
   log("using following command for pipe: " + cmd);
   FILE* c_pipe = popen(cmd.c_str(), pipeMode());
   if ( !c_pipe ) {
      if ( errno != 0 )
         perror("System error opening opipestream");
         error(ETFatal, "unable to open pipe for cmd: %s", cmd.c_str());
   }
                        
   std_buffer* tmp = new std_buffer(c_pipe, bufferMode());
   assert(tmp != NULL);

   stream = stream_type(tmp, PIPE_deleter(c_pipe));
}

// A unified way of opening file
void MagicStreamBase::makeFile(const string& filename)
{
   log("Using files");
   std::filebuf* tmp = new std::filebuf;
   assert(tmp);
   tmp->open(filename.c_str(), bufferMode());
   stream = stream_type(tmp, close_deleter());
}

bool MagicStreamBase::isBzip2(const string& cmd)
{
   return cmd.rfind(".") != string::npos
          && (cmd.substr(cmd.rfind(".")) == ".bz"
              || cmd.substr(cmd.rfind(".")) == ".bzip2"
              || cmd.substr(cmd.rfind(".")) == ".bz2");
}

bool MagicStreamBase::isZip(const string& cmd)
{
   // Unsupported extensions are: -gz, -z, _z
   return cmd.rfind(".") != string::npos
          && (cmd.substr(cmd.rfind(".")) == ".gz"
              || cmd.substr(cmd.rfind(".")) == ".z"
              || cmd.substr(cmd.rfind(".")) == ".Z");
}

void MagicStreamBase::setQuiet(bool bQuiet)
{
   Quiet = bQuiet;
}


//////////////////////////////////////////////////////////////////////////////
// iMagicStream
iMagicStream::iMagicStream()
: MagicStreamBase("r", ios_base::in), istream(0)
{}

iMagicStream::iMagicStream(const string& s, bool bQuiet)
: MagicStreamBase("r", ios_base::in), istream(0)
{
   setQuiet(bQuiet);
   open(s);
}

iMagicStream::iMagicStream(int fd, bool closeAtEnd)
: MagicStreamBase("r", ios_base::in, fd, closeAtEnd), istream(0)
{
   init(stream.get());
}

iMagicStream::iMagicStream(FILE* f, bool closeAtEnd)
: MagicStreamBase("r", ios_base::in, f, closeAtEnd), istream(0)
{
   init(stream.get());
}

iMagicStream::~iMagicStream()
{}

void iMagicStream::open(const string& s)
{
   assert(!s.empty());
   if (s.empty())
      error(ETFatal, "You have provided an empty filename in iMagicStream");

   log("iMagicStream::open with: " + s);
   if (s == "-") {
      log("Using stdin");
      stream = stream_type(cin.rdbuf(), null_deleter());
   }
   else if (s.length()>0 && *s.rbegin() == '|') {
      makePipe(s.substr(0, s.length()-1));
   }
   else if (isBzip2(s)) {
      std::filebuf* tmp = new std::filebuf;
      assert(tmp);
      if (tmp->open(s.c_str(), bufferMode())) {
         tmp->close(); // File exists we must close it to now use it with gzip
         delete tmp; tmp = NULL;
         makePipe("bzip2 -cqdf " + s);
      }
   }
   else if (isZip(s)) {
      std::filebuf* tmp = new std::filebuf;
      assert(tmp);
      if (tmp->open(s.c_str(), bufferMode())) {
         tmp->close(); // File exists we must close it to now use it with gzip
         delete tmp; tmp = NULL;
         makePipe("gzip -cqdf " + s);
      }
   }
   else {
      //makeFile(s);
      log("Using files");
      std::filebuf* tmp = new std::filebuf;
      assert(tmp);
      if (tmp->open(s.c_str(), bufferMode())) {
         stream = stream_type(tmp, close_deleter());
      }
      else {
         // If it fails to open a file we will try its gzip version
         const string filename(s + ".gz");
         if (tmp->open(filename.c_str(), bufferMode())) {
            //error(ETWarn, "Unable to open file: %s.  Opening its .gz form instead.",
            //      s.c_str(), s.c_str());
            tmp->close(); // File exists we must close it to now use it with gzip
            delete tmp; tmp = NULL;
            makePipe("gzip -cqdf " + filename);
         }
      }
   }
      
   init(stream.get());
}


//////////////////////////////////////////////////////////////////////////////
// oMagicStream
oMagicStream::oMagicStream()
: MagicStreamBase("w", ios_base::out), ostream(0)
{}

oMagicStream::oMagicStream(const string& s, bool bQuiet)
: MagicStreamBase("w", ios_base::out), ostream(0)
{
   setQuiet(bQuiet);
   open(s);
}

oMagicStream::oMagicStream(int fd, bool closeAtEnd)
: MagicStreamBase("w", ios_base::out, fd, closeAtEnd), ostream(0)
{
   init(stream.get());
}

oMagicStream::oMagicStream(FILE* f, bool closeAtEnd)
: MagicStreamBase("w", ios_base::out, f, closeAtEnd), ostream(0)
{
   init(stream.get());
}

oMagicStream::~oMagicStream()
{
   log("oMagicStream::~oMagicStream");
   if (stream.get() != NULL) flush();
}

void oMagicStream::open(const string& s)
{
   if (s.empty())
      error(ETFatal, "You have provided an empty filename in oMagicStream");

   log("oMagicStream::open with: " + s);
   if (s == "-") {
      log("Using stdout");
      stream = stream_type(cout.rdbuf(), null_deleter());
   }
   else if (s.length()>0 && s[0] == '|') {
      makePipe(s.substr(1));
   }
   else if (isBzip2(s)) {
      string command("bzip2 -cqf > ");
      command += s;
      makePipe(command);
   }
   else if (isZip(s)) {
      string command("gzip -cqf > ");
      command += s;
      makePipe(command);
   }
   else {
      makeFile(s);
      // A one-liner way of creating the filebuf
      //stream = stream_type((new std::filebuf)->open(s.c_str(), std::ios::out));
   }
      
   init(stream.get());
}
