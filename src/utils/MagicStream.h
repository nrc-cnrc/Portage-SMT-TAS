/**
 * @author Samuel Larkin
 * @file MagicStream.h A stream that can be transparently used for cin/cout,
 *                     .txt, .{Z,z,gz}, .bz2, .lzma and pipes.
 *
 *
 * COMMENTS: These classes we permit easy integration of a stream that can be
 *           used in the following ways:
 *           - to read from standard in or to write to standard out
 *           - to read/write to a compress gzip file
 *           - to read/write to a compress bzip2 file
 *           - to read/write to a compress lzma file
 *           - to read/write to a plain text file
 *           - to read/write from a pipe
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
           
#ifndef __MAGIC_STREAM_H__
#define __MAGIC_STREAM_H__

#include <iostream>
#include <boost/shared_ptr.hpp>
#include <ext/stdio_filebuf.h>
#include <fstream>
#include <string>
#include <cerrno>
#include <boost/noncopyable.hpp>


namespace Portage {

/**
 * Abstract base class for MagicStreams.  Not intended to be instanciated as a
 * concrete object but rather as a placeholder of common MagicStream
 * funcion/members.
 */
class MagicStreamBase : private boost::noncopyable
{
   protected:
      /// Internal buffer's definition
      typedef boost::shared_ptr<std::streambuf> stream_type;
      
      /// Internal buffer.  A shared pointer will hold our reference to our
      /// buffer with the proper deleter will automagically close the buffer
      stream_type stream;

      /// Opening mode's definition (in | out)
      typedef std::ios_base::openmode  OpenMode;

      /// Opening pipe mode's definition (r|w)
      typedef const char* PipeMode;
      
   private:
      /// open pipe mode ("r" | "w")
      const PipeMode  m_pipeMode;
      /// open stream mode (ios_base::in | ios_base::out)
      const OpenMode  m_bufferMode;  
      /// Quiet mode status
      bool Quiet;
      
      
   protected:
      /**
       * Constructor.  Minimal constructor.
       * @param _p  Open pipe mode r|w
       * @param _b  Open mode in|out
       */
      MagicStreamBase(const PipeMode _p, const OpenMode _b);

      /**
       * Constructor from an opened file descriptor.
       * @param _p  Open pipe mode r|w
       * @param _b  Open mode in|out
       * @param fd  file descriptor of an opened file.
       * @param closeAtEnd tell MagicStream to close the file descriptor.
       */
      MagicStreamBase(const PipeMode _p, const OpenMode _b, int fd, bool closeAtEnd = false);

      /**
       * Constructor from an opened file handle.
       * @param _p  Open pipe mode r|w
       * @param _b  Open mode in|out
       * @param f   file handle of an opened file.
       * @param closeAtEnd tell MagicStream to close the file handle.
       */
      MagicStreamBase(const PipeMode _p, const OpenMode _b, FILE* f, bool closeAtEnd = false);

      /// Destructor.  All the magic happens in the stream_type which will
      /// automagically properly close the internal buffer.
      virtual ~MagicStreamBase();
      
      /// Determines the open pipe mode (r or w).
      /// @return Returns the open pipe mode.
      virtual const PipeMode pipeMode() const;
      
      /// Determines the open mode (in or out).
      /// @return Returns the open mode.
      virtual const OpenMode bufferMode() const;

      /// A unified way of opening a pipe.
      /// @param cmd the command to open in pipe mode.
      /// @return true if the pipe was opened successfully, false if it failed
      ///         due to lack of memory.  does error(ETFatal) if it fails for
      ///         other reasons.
      bool makePipe(const std::string& cmd);

      /// A unified way of opening file.
      /// @param filename filename to open.
      void makeFile(const std::string& filename);

   public:
      /**
       * Determines if the cmd has a \.gz or \.Z  extension.
       * @param cmd command to check if it ends in .gz or .Z
       * @return Returns true if cmd ends with .gz or .Z
       */
      static bool isZip(const std::string& cmd);

      /**
       * Determines if the cmd is a bzip2 operation.
       * @param cmd command to check if it ends in .bz, .bz2 or .bzip2
       * @return Returns true if cmd ends with .bz, .bz2 or .bzip2
       */
      static bool isBzip2(const std::string& cmd);

      /**
       * Determines if the cmd is a lzma operation.
       * @param cmd command to check if it ends in .lzma
       * @return Returns true if cmd ends with .lzma
       */
      static bool isLzma(const std::string& cmd);

      /**
       * Opens the proper stream.  This virtual function must be declared by
       * oMagicStream and iMagicStream since opening is different in both
       * cases.
       * @param s what to open (filename, pipe, zip or standard i/o).
       */
      virtual void open(const std::string& s) = 0;

      /// Closes the buffer.  Renders the stream invalid.
      virtual void close();

      /// Checks if stream is open.
      /// @return Returns true if stream is open
      virtual bool is_open() const;

      /**
       * Turn quiet mode on or off.
       * @param bQuiet  quiet or not
       */
      void setQuiet(bool bQuiet);
};

/// Input Magic Stream.  Transparently read from regular files, compressed
/// files, pipes, stdin.
class iMagicStream : public MagicStreamBase, public std::istream
{
   public:
      /// Default constructor.
      /// You should probably use iMagicStream(const std::string& s).
      iMagicStream();

      /// Constructor that opens the stream.
      /// @param s command to open
      /// @param bQuiet makes the stream quiet for Broken Pipes
      iMagicStream(const std::string& s, bool bQuiet = false);

      /// Constructor that opens the stream from an opened file descriptor.
      /// @param fd file descriptor.
      /// @param closeAtEnd tell MagicStream to close the file descriptor.
      iMagicStream(int fd, bool closeAtEnd = false);

      /// Constructor that opens the stream from an opened file handle.
      /// @param f file handle.
      /// @param closeAtEnd tell MagicStream to close the file handle.
      iMagicStream(FILE* f, bool closeAtEnd = false);

      /// Destructor that automagically and properly close the stream/buffer.
      virtual ~iMagicStream();
      
      /// Opens the stream/buffer.
      /// @param s comand to open
      virtual void open(const std::string& s);
};

/// Output Magic Stream.  Transparently create - and write to - regular files,
/// compressed files, pipes and stdout.
class oMagicStream : public MagicStreamBase, public std::ostream
{
   public:
      /// Default constructor.
      /// You should probably use oMagicStream(const std::string& s).
      oMagicStream();

      /// Constructor that opens the stream.
      /// @param s command to open
      /// @param bQuiet makes the stream quiet for Broken Pipes
      oMagicStream(const std::string& s, bool bQuiet = false);

      /// Constructor that opens the stream from an opened file descriptor.
      /// @param fd file descriptor.
      /// @param closeAtEnd tell MagicStream to close the file descriptor.
      oMagicStream(int fd, bool closeAtEnd = false);

      /// Constructor that opens the stream from an opened file handle.
      /// @param f file handle.
      /// @param closeAtEnd tell MagicStream to close the file handle.
      oMagicStream(FILE* f, bool closeAtEnd = false);

      /// Destructor that automagically and properly close the stream/buffer.
      virtual ~oMagicStream();
      
      /// Opens the stream/buffer.
      /// @param s comand to open
      virtual void open(const std::string& s);
};
} // ends Portage

#endif // __MAGIC_STREAM_H__

