/*
 * Copyright (c) 2005-2008  cielacanth <cielacanth AT s60.xrea.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __LLDEBUG_CONFIGFILE__
#define __LLDEBUG_CONFIGFILE__

#include <boost/filesystem/path.hpp>
#include <fstream>

namespace lldebug {

/// Transform filename that may contain all characters such as ".\!#$%/&'()"
/// to string that can use a filename.
std::string EncodeToFilename(const std::string &filename);

/// Get the filepath located on the config dir.
boost::filesystem::path GetConfigFilePath(const std::string &filename);

/// Get the filepath(std::string) located on the config dir.
std::string GetConfigFileName(const std::string &filename);


/**
 * @brief Save file only when the output was successed.
 */
class safe_ofstream {
public:
	explicit safe_ofstream();
	~safe_ofstream();

	/// Open the file.
	bool open(const std::string &filename, std::ios_base::openmode mode);

	/// Is this stream open ?
	bool is_open() const;

	/// The temporary file is closed and renamed to the target name.
	void commit();

	/// The tmpoyrary file is deleted.
	void discard();

	/// Get the ofstream object.
	std::ofstream &stream() {
		return m_stream;
	}

private:
	std::ofstream m_stream;
	boost::filesystem::path m_filePath;
	boost::filesystem::path m_tmpPath;
};

} // end of namespace lldebug

#endif
