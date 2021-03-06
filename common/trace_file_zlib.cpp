/**************************************************************************
 *
 * Copyright 2011 Zack Rusin
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/


#include "trace_file.hpp"


#include <assert.h>
#include <string.h>

#include <zlib.h>

#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include "os.hpp"

#include <iostream>


using namespace trace;


class ZLibFile : public File {
public:
    ZLibFile(const std::string &filename = std::string());
    virtual ~ZLibFile();


    virtual bool supportsOffsets() const;
    virtual File::Offset currentOffset();
protected:
    virtual bool rawOpen(const std::string &filename);
    virtual size_t rawRead(void *buffer, size_t length);
    virtual int rawGetc();
    virtual void rawClose();
    virtual bool rawSkip(size_t length);
    virtual int  rawPercentRead();
private:
    int fd;
    gzFile m_gzFile;
    double m_endOffset;
};

ZLibFile::ZLibFile(const std::string &filename)
    : File(filename),
      m_gzFile(NULL)
{
}

ZLibFile::~ZLibFile()
{
    close();
}

bool ZLibFile::rawOpen(const std::string &filename)
{
    int flags = O_RDONLY;
#ifdef O_BINARY
    flags |= O_BINARY;
#endif
#ifdef O_LARGEFILE
    flags |= O_LARGEFILE;
#endif

#ifdef _WIN32
    fd = _open(filename.c_str(), flags, 0666);
#else
    fd = ::open(filename.c_str(), flags, 0666);
#endif
    if (fd < 0) {
        return false;
    }

    m_gzFile = gzdopen(fd, "rb");

    if (m_gzFile) {
        //XXX: unfortunately zlib doesn't support
        //     SEEK_END or we could've done:
        //m_endOffset = gzseek(m_gzFile, 0, SEEK_END);
        //gzrewind(m_gzFile);
        off_t loc = lseek(fd, 0, SEEK_CUR);
        m_endOffset = lseek(fd, 0, SEEK_END);
        lseek(fd, loc, SEEK_SET);
    }

    return m_gzFile != NULL;
}

size_t ZLibFile::rawRead(void *buffer, size_t length)
{
    int ret = gzread(m_gzFile, buffer, unsigned(length));
    return ret < 0 ? 0 : ret;
}

int ZLibFile::rawGetc()
{
    return gzgetc(m_gzFile);
}

void ZLibFile::rawClose()
{
    if (m_gzFile) {
        gzclose(m_gzFile);
        m_gzFile = NULL;
    }
}

File::Offset ZLibFile::currentOffset()
{
    return File::Offset(gztell(m_gzFile));
}

bool ZLibFile::supportsOffsets() const
{
    return false;
}

bool ZLibFile::rawSkip(size_t)
{
    return false;
}

int ZLibFile::rawPercentRead()
{
    return int(100 * (lseek(fd, 0, SEEK_CUR) / m_endOffset));
}


File * File::createZLib(void) {
    return new ZLibFile;
}
