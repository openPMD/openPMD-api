/* Copyright 2017-2021 Franz Poeschel.
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "openPMD/IO/InvalidatableFile.hpp"

openPMD::InvalidatableFile::InvalidatableFile(std::string s)
    : fileState{std::make_shared<FileState>(s)}
{}

void openPMD::InvalidatableFile::invalidate()
{
    fileState->valid = false;
}

bool openPMD::InvalidatableFile::valid() const
{
    return fileState->valid;
}

openPMD::InvalidatableFile &openPMD::InvalidatableFile::operator=(std::string s)
{
    if (fileState)
    {
        fileState->name = s;
    }
    else
    {
        fileState = std::make_shared<FileState>(s);
    }
    return *this;
}

bool openPMD::InvalidatableFile::operator==(
    const openPMD::InvalidatableFile &f) const
{
    return this->fileState == f.fileState;
}

std::string &openPMD::InvalidatableFile::operator*() const
{
    return fileState->name;
}

std::string *openPMD::InvalidatableFile::operator->() const
{
    return &fileState->name;
}

openPMD::InvalidatableFile::operator bool() const
{
    return fileState.operator bool();
}

openPMD::InvalidatableFile::FileState::FileState(std::string s)
    : name{std::move(s)}
{}

std::hash<openPMD::InvalidatableFile>::result_type
std::hash<openPMD::InvalidatableFile>::operator()(
    const openPMD::InvalidatableFile &s) const noexcept
{
    return std::hash<shared_ptr<openPMD::InvalidatableFile::FileState>>{}(
        s.fileState);
}
