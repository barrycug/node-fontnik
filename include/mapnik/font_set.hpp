/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2011 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

#pragma once

// stl
#include <string>
#include <vector>

namespace mapnik {

class font_set {
public:
    font_set(std::string const& name);
    font_set(font_set const& rhs);
    ~font_set();

    font_set& operator=(font_set const& rhs);

    std::size_t size() const;
    void set_name(std::string const& name);
    std::string const& get_name() const;
    void add_face_name(std::string const& face_name);
    void add_fontstack(std::string const& fontstack, char delim);
    std::vector<std::string> const& get_face_names() const;
private:
    std::string name_;
    std::vector<std::string> face_names_;
    std::string trim(std::string const& str,
                     std::string const& whitespace = " \t");
};

}