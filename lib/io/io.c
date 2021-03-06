/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <lib/io.h>

#include <uapi/err.h>
#include <ctype.h>
#include <assert.h>

ssize_t io_write(io_handle_t *io, const char *buf, size_t len)
{
    DEBUG_ASSERT(io->magic == IO_HANDLE_MAGIC);

    if (!io->hooks->write)
        return ERR_NOT_SUPPORTED;

    return io->hooks->write(io, buf, len);
}

void io_write_commit(io_handle_t *io)
{
    DEBUG_ASSERT(io->magic == IO_HANDLE_MAGIC);

    if (io->hooks->write_commit)
        io->hooks->write_commit(io);
}

void io_lock(io_handle_t *io)
{
    DEBUG_ASSERT(io->magic == IO_HANDLE_MAGIC);

    if (io->hooks->lock)
        io->hooks->lock(io);
}

void io_unlock(io_handle_t *io)
{
    DEBUG_ASSERT(io->magic == IO_HANDLE_MAGIC);

    if (io->hooks->unlock)
        io->hooks->unlock(io);
}

ssize_t io_read(io_handle_t *io, char *buf, size_t len)
{
    DEBUG_ASSERT(io->magic == IO_HANDLE_MAGIC);

    if (!io->hooks->read)
        return ERR_NOT_SUPPORTED;

    return io->hooks->read(io, buf, len);
}
