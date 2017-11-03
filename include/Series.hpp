/* Copyright 2017 Fabian Koller
 *
 * This file is part of libopenPMD.
 *
 * libopenPMD is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libopenPMD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with libopenPMD.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once


#include "Attributable.hpp"
#include "Container.hpp"
#include "IO/AbstractIOHandler.hpp"
#include "IO/AccessType.hpp"
#include "IO/Format.hpp"
#include "Iteration.hpp"
#include "Writable.hpp"


enum class IterationEncoding
{
    fileBased, groupBased
};  //IterationEncoding


/** @brief  Root level of the openPMD hierarchy.
 *
 * Entry point and common link between all iterations of particle and mesh data.
 *
 * @see https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file
 * @see https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series
 */
class Series : public Attributable
{
public:
    /** Detailed constructor for series of iterations.
     *
     * @param path  Directory of the data, relative(!) to the current working directory.
     * @param name  Pattern for file names. Should include iteration regex %T if iterationEncoding is filesBased.
     * @param ie    <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series">iterationEncoding</A> of multiple iterations in this series.
     * @param f     File format and IO backend to use for this series.
     * @param at    Access permissions for every file in this series.
     */
    static Series create(std::string const& path,
                         std::string const& name,
                         IterationEncoding ie,
                         Format f,
                         AccessType at = AccessType::CREAT);
    /** Convenience constructor for read-only data.
     *
     * @param path      Directory of the data, relative(!) to the current working directory.
     * @param name      Concrete name of one file in series (fileBased series will be loaded according to iterationFormat). Must include file extension.
     * @param parallel  Flag indicating whether this series should be read with an MPI-compatible backend. If <CODE>true</CODE>, <CODE>MPI_init()</CODE> must be called before.
     */
    static Series read(std::string const& path,
                       std::string const& name,
                       bool readonly = true,
                       bool parallel = false);
    ~Series();

    /**
     * @return  String representing the current enforced version of the <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file">openPMD standard</A>.
     */
    std::string openPMD() const;
    /** Set the version of the enforced <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file">openPMD standard</A>.
     *
     * @param   openPMD   String <CODE>MAJOR.MINOR.REVISION</CODE> of the desired version of the openPMD standard.
     * @return  Reference to modified series.
     */
    Series& setOpenPMD(std::string const& openPMD);

    /**
     * @return  32-bit mask of applied extensions to the <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file">openPMD standard</A>.
     */
    uint32_t openPMDextension() const;
    /** Set a 32-bit mask of applied extensions to the <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#hierarchy-of-the-data-file">openPMD standard</A>.
     *
     * @param   openPMDextension  Unsigned 32-bit integer used as a bit-mask of applied extensions.
     * @return  Reference to modified series.
     */
    Series& setOpenPMDextension(uint32_t openPMDextension);

    /**
     * @return  String representing the common prefix for all data sets and sub-groups of a specific iteration.
     */
    std::string basePath() const;
    /** Set the common prefix for all data sets and sub-groups of a specific iteration.
     *
     * @param   basePath    String of the common prefix for all data sets and sub-groups of a specific iteration.
     * @return  Reference to modified series.
     */
    Series& setBasePath(std::string const& basePath);

    /**
     * @return  String representing the path to mesh records, relative(!) to <CODE>basePath</CODE>.
     */
    std::string meshesPath() const;
    /** Set the path to <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#mesh-based-records">mesh records</A>, relative(!) to <CODE>basePath</CODE>.
     *
     * @param   meshesPath  String of the path to <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#mesh-based-records">mesh records</A>, relative(!) to <CODE>basePath</CODE>.
     * @return  Reference to modified series.
     */
    Series& setMeshesPath(std::string const& meshesPath);

    /**
     * @return  String representing the path to particle species, relative(!) to <CODE>basePath</CODE>.
     */
    std::string particlesPath() const;
    /** Set the path to groups for each <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#particle-records">particle species</A>, relative(!) to <CODE>basePath</CODE>.
     *
     * @param   particlesPath   String of the path to groups for each <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#particle-records">particle species</A>, relative(!) to <CODE>basePath</CODE>.
     * @return  Reference to modified series.
     */
    Series& setParticlesPath(std::string const& particlesPath);

    /**
     * @throw   no_such_attribute_error If optional attribute is not present.
     * @return  String indicating author and contact for the information in the file.
     */
    std::string author() const;
    /** Indicate the author and contact for the information in the file.
     *
     * @param   author  String indicating author and contact for the information in the file.
     * @return  Reference to modified series.
     */
    Series& setAuthor(std::string const& author);

    /**
     * @throw   no_such_attribute_error If optional attribute is not present.
     * @return  String indicating the software/code/simulation that created the file;
     */
    std::string software() const;
    /** Indicate the software/code/simulation that created the file.
     *
     * @param   software    String indicating the software/code/simulation that created the file.
     * @return  Reference to modified series.
     */
    Series& setSoftware(std::string const& software);

    /**
     * @throw   no_such_attribute_error If optional attribute is not present.
     * @return  String indicating the version of the software/code/simulation that created the file.
     */
    std::string softwareVersion() const;
    /** Indicate the version of the software/code/simulation that created the file.
     *
     * @param   softwareVersion String inidating the version of the software/code/simulation that created the file.
     * @return  Reference to modified series.
     */
    Series& setSoftwareVersion(std::string const& softwareVersion);

    /**
     * @throw   no_such_attribute_error If optional attribute is not present.
     * @return  String inidcating date of creation.
     */
    std::string date() const;
    /** Indicate the date of creation.
     *
     * @param   date    String indication the date of creation.
     * @return  Reference to modified series.
     */
    Series& setDate(std::string const& date);

    /**
     * @return  Current encoding style for multiple iterations in this series.
     */
    IterationEncoding iterationEncoding() const;
    /** Set the <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series">encoding style</A> for multiple iterations in this series.
     *
     * @param   iterationEncoding   Desired <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series">encoding style</A> for multiple iterations in this series.
     * @return  Reference to modified series.
     */
    Series& setIterationEncoding(IterationEncoding iterationEncoding);

    /**
     * @return  String describing a <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series">pattern</A> describing how to access single iterations in the raw file.
     */
    std::string iterationFormat() const;
    /** Set a <A HREF="https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#iterations-and-time-series">pattern</A> describing how to access single iterations in the raw file.
     *
     * @param   iterationFormat String with the iteration regex <CODE>\%T</CODE> defining either
     *                          the series of files (fileBased)
     *                          or the series of groups within a single file (groupBased)
     *                          that allows to extract the iteration from it.\n
     *                          For fileBased formats the iteration must be included in the file name.\n
     *                          The format depends on the selected iterationEncoding method.
     * @return  Reference to modified series.
     */
    Series& setIterationFormat(std::string const& iterationFormat);

    /**
     * @return  String of a pattern for file names.
     */
    std::string name() const;
    /** Set the pattern for file names.
     *
     * @param   name    String of the pattern for file names. Must include iteration regex <CODE>\%T</CODE> for fileBased data.
     * @return  Reference to modified series.
     */
    Series& setName(std::string const& name);

    /** Execute all required remaining IO operations to write or read data.
     */
    void flush();

    Container< Iteration, uint64_t > iterations;

private:
    // TODO replace entirely with factory
    Series(std::string const& path,
           std::string const& name,
           IterationEncoding ie,
           Format f,
           AccessType at);

    // TODO replace entirely with factory
    Series(std::string path,
           std::string const& name,
           bool readonly,
           bool parallel);

    void flushFileBased();
    void flushGroupBased();
    void readFileBased();
    void readGroupBased();
    void read();

    static std::string cleanFilename(std::string, Format);

    constexpr static char const * const BASEPATH = "/data/%T/";
    constexpr static char const * const OPENPMD = "1.0.1";

    IterationEncoding m_iterationEncoding;
    std::string m_name;
};  //Series

std::ostream&
operator<<(std::ostream&, IterationEncoding);
