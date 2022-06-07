/* Copyright 2018-2021 Fabian Koller
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
#pragma once

#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/IOTask.hpp"
#include "openPMD/auxiliary/DerefDynamicCast.hpp"

#include <future>
#include <iostream>

namespace openPMD
{
// class AbstractIOHandler;
class Writable;

class AbstractIOHandlerImpl
{
public:
    AbstractIOHandlerImpl(AbstractIOHandler *handler) : m_handler{handler}
    {}

    virtual ~AbstractIOHandlerImpl() = default;

    std::future<void> flush()
    {
        using namespace auxiliary;

        while (!(*m_handler).m_work.empty())
        {
            IOTask &i = (*m_handler).m_work.front();
            try
            {
                switch (i.operation)
                {
                    using O = Operation;
                case O::CREATE_FILE:
                    createFile(
                        i.writable,
                        deref_dynamic_cast<Parameter<Operation::CREATE_FILE> >(
                            i.parameter.get()));
                    break;
                case O::CREATE_PATH:
                    createPath(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::CREATE_PATH> >(
                            i.parameter.get()));
                    break;
                case O::CREATE_DATASET:
                    createDataset(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::CREATE_DATASET> >(
                            i.parameter.get()));
                    break;
                case O::EXTEND_DATASET:
                    extendDataset(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::EXTEND_DATASET> >(
                            i.parameter.get()));
                    break;
                case O::OPEN_FILE:
                    openFile(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::OPEN_FILE> >(
                            i.parameter.get()));
                    break;
                case O::CLOSE_FILE:
                    closeFile(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::CLOSE_FILE> >(
                            i.parameter.get()));
                    break;
                case O::OPEN_PATH:
                    openPath(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::OPEN_PATH> >(
                            i.parameter.get()));
                    break;
                case O::CLOSE_PATH:
                    closePath(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::CLOSE_PATH> >(
                            i.parameter.get()));
                    break;
                case O::OPEN_DATASET:
                    openDataset(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::OPEN_DATASET> >(
                            i.parameter.get()));
                    break;
                case O::DELETE_FILE:
                    deleteFile(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::DELETE_FILE> >(
                            i.parameter.get()));
                    break;
                case O::DELETE_PATH:
                    deletePath(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::DELETE_PATH> >(
                            i.parameter.get()));
                    break;
                case O::DELETE_DATASET:
                    deleteDataset(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::DELETE_DATASET> >(
                            i.parameter.get()));
                    break;
                case O::DELETE_ATT:
                    deleteAttribute(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::DELETE_ATT> >(
                            i.parameter.get()));
                    break;
                case O::WRITE_DATASET:
                    writeDataset(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::WRITE_DATASET> >(
                            i.parameter.get()));
                    break;
                case O::WRITE_ATT:
                    writeAttribute(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::WRITE_ATT> >(
                            i.parameter.get()));
                    break;
                case O::READ_DATASET:
                    readDataset(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::READ_DATASET> >(
                            i.parameter.get()));
                    break;
                case O::GET_BUFFER_VIEW:
                    getBufferView(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::GET_BUFFER_VIEW> >(
                            i.parameter.get()));
                    break;
                case O::READ_ATT:
                    readAttribute(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::READ_ATT> >(
                            i.parameter.get()));
                    break;
                case O::LIST_PATHS:
                    listPaths(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::LIST_PATHS> >(
                            i.parameter.get()));
                    break;
                case O::LIST_DATASETS:
                    listDatasets(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::LIST_DATASETS> >(
                            i.parameter.get()));
                    break;
                case O::LIST_ATTS:
                    listAttributes(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::LIST_ATTS> >(
                            i.parameter.get()));
                    break;
                case O::ADVANCE:
                    advance(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::ADVANCE> >(
                            i.parameter.get()));
                    break;
                case O::AVAILABLE_CHUNKS:
                    availableChunks(
                        i.writable,
                        deref_dynamic_cast<Parameter<O::AVAILABLE_CHUNKS> >(
                            i.parameter.get()));
                    break;
                }
            }
            catch (...)
            {
                std::cerr << "[AbstractIOHandlerImpl] IO Task "
                          << internal::operationAsString(i.operation)
                          << " failed with exception. Removing task"
                          << " from IO queue and passing on the exception."
                          << std::endl;
                (*m_handler).m_work.pop();
                throw;
            }
            (*m_handler).m_work.pop();
        }
        return std::future<void>();
    }

    /**
     * Close the file corresponding with the writable and release file handles.
     * The operation should succeed in any access mode.
     */
    virtual void
    closeFile(Writable *, Parameter<Operation::CLOSE_FILE> const &) = 0;

    /** Advance the file/stream that this writable belongs to.
     *
     * If the backend is based around usage of IO steps (especially streaming
     * backends), open or close an IO step. This is modeled closely after the
     * step concept in ADIOS2.
     *
     * This task is used to implement streaming-aware semantics in the openPMD
     * API by splitting data into packets that are written to and read from
     * transport.
     *
     * IO actions up to the point of closing a step must be performed now.
     *
     * The advance mode is determined by parameters.mode.
     * The return status code shall be stored as parameters.status.
     */
    virtual void advance(Writable *, Parameter<Operation::ADVANCE> &)
    {}

    /** Close an openPMD group.
     *
     * This is an optimization-enabling task and may be ignored by backends.
     * Indicates that the group will not be accessed any further.
     * Especially in step-based IO mode (e.g. streaming):
     * Indicates that the group corresponding with the writable needs not be
     * held in a parseable state for this and upcoming IO steps, allowing for
     * deletion of metadata to be sent/stored (attributes, datasets, ..). Should
     * fail if the writable is not written. Should fail if m_handler->accessType
     * is AccessType::READ_ONLY.
     *
     */
    virtual void closePath(Writable *, Parameter<Operation::CLOSE_PATH> const &)
    {}

    /** Report chunks that are available for loading from the dataset
     * represented by this writable.
     *
     * The resulting chunks should be stored into parameters.chunks.
     *
     */
    virtual void
    availableChunks(Writable *, Parameter<Operation::AVAILABLE_CHUNKS> &) = 0;

    /** Create a new file in physical storage, possibly overriding an existing
     * file.
     *
     * The operation should fail if m_handler->m_frontendAccess is
     * Access::READ_ONLY. The new file should be located in
     * m_handler->directory. The new file should have the filename
     * parameters.name. The filename should include the correct corresponding
     * filename extension. Any existing file should be overwritten if
     * m_handler->m_frontendAccess is Access::CREATE. The Writables file
     * position should correspond to the root group "/" of the hierarchy. The
     * Writable should be marked written when the operation completes
     * successfully.
     */
    virtual void
    createFile(Writable *, Parameter<Operation::CREATE_FILE> const &) = 0;
    /** Create all necessary groups for a path, possibly recursively.
     *
     * The operation should fail if m_handler->m_frontendAccess is
     * Access::READ_ONLY. The path parameters.path may contain multiple levels
     * (e.g. first/second/third/). The Writables file position should correspond
     * to the complete newly created path (i.e. first/second/third/ should be
     * assigned to the Writables file position). The Writable should be marked
     * written when the operation completes successfully.
     */
    virtual void
    createPath(Writable *, Parameter<Operation::CREATE_PATH> const &) = 0;
    /** Create a new dataset of given type, extent and storage properties.
     *
     * The operation should fail if m_handler->m_frontendAccess is
     * Access::READ_ONLY. The path may contain multiple levels (e.g.
     * group/dataset). The new dataset should have the name parameters.name.
     * This name should not start or end with a slash ("/"). The new dataset
     * should be of datatype parameters.dtype. The new dataset should have an
     * extent of parameters.extent. If possible, the new dataset should be
     * extensible. If possible, the new dataset should be divided into chunks
     * with size parameters.chunkSize. If possible, the new dataset should be
     * compressed according to parameters.compression. This may be
     * format-specific. If possible, the new dataset should be transformed
     * accoring to parameters.transform. This may be format-specific. The
     * Writables file position should correspond to the newly created dataset.
     * The Writable should be marked written when the operation completes
     * successfully.
     */
    virtual void
    createDataset(Writable *, Parameter<Operation::CREATE_DATASET> const &) = 0;
    /** Increase the extent of an existing dataset.
     *
     * The operation should fail if m_handler->m_frontendAccess is
     * Access::READ_ONLY. The operation should fail if the dataset does not yet
     * exist. The dataset should have the name parameters.name. This name should
     * not start or end with a slash ("/"). The operation should fail if the new
     * extent is not strictly large in every dimension. The dataset should have
     * an extent of parameters.extent.
     */
    virtual void
    extendDataset(Writable *, Parameter<Operation::EXTEND_DATASET> const &) = 0;
    /** Open an existing file assuming it conforms to openPMD.
     *
     * The operation should fail if m_handler->directory is not accessible.
     * The opened file should have filename parameters.name and include the
     * correct corresponding filename extension. The operation should not open
     * files more than once. If possible, the file should be opened with
     * read-only permissions if m_handler->m_frontendAccess is
     * Access::READ_ONLY. The Writables file position should correspond to the
     * root group "/" of the hierarchy in the opened file. The Writable should
     * be marked written when the operation completes successfully.
     */
    virtual void
    openFile(Writable *, Parameter<Operation::OPEN_FILE> const &) = 0;
    /** Open all contained groups in a path, possibly recursively.
     *
     * The operation should overwrite existing file positions, even when the
     * Writable was already marked written. The path parameters.path may contain
     * multiple levels (e.g. first/second/third/). This path should be relative
     * (i.e. it should not start with a slash "/"). The number of levels may be
     * zero, i.e. parameters.path may be an empty string. The Writables file
     * position should correspond to the complete opened path (i.e.
     * first/second/third/ should be assigned to the Writables file position).
     * The Writable should be marked written when the operation completes
     * successfully.
     */
    virtual void
    openPath(Writable *, Parameter<Operation::OPEN_PATH> const &) = 0;
    /** Open an existing dataset and determine its datatype and extent.
     *
     * The opened dataset should be located in a group below the group of the
     * Writables parent writable->parent. The opened datasets name should be
     * parameters.name. This name should not start or end with a slash ("/").
     * The opened datasets datatype should be stored in *(parameters.dtype).
     * The opened datasets extent should be stored in *(parameters.extent).
     * The Writables file position should correspond to the opened dataset.
     * The Writable should be marked written when the operation completes
     * successfully.
     */
    virtual void
    openDataset(Writable *, Parameter<Operation::OPEN_DATASET> &) = 0;
    /** Delete an existing file from physical storage.
     *
     * The operation should fail if m_handler->m_frontendAccess is
     * Access::READ_ONLY. The operation should pass if the Writable was not
     * marked written. All handles that correspond to the file should be closed
     * before deletion. The file to delete should have the filename
     * parameters.name. The filename should include the correct corresponding
     * filename extension. The Writables file position should be set to an
     * invalid position (i.e. the pointer should be a nullptr). The Writable
     * should be marked not written when the operation completes successfully.
     */
    virtual void
    deleteFile(Writable *, Parameter<Operation::DELETE_FILE> const &) = 0;
    /** Delete all objects within an existing path.
     *
     * The operation should fail if m_handler->m_frontendAccess is
     * Access::READ_ONLY. The operation should pass if the Writable was not
     * marked written. The path parameters.path may contain multiple levels
     * (e.g. first/second/third/). This path should be relative (i.e. it should
     * not start with a slash "/"). It may also contain the current group ".".
     * All groups and datasets starting from the path should not be accessible
     * in physical storage after the operation completes successfully. The
     * Writables file position should be set to an invalid position (i.e. the
     * pointer should be a nullptr). The Writable should be marked not written
     * when the operation completes successfully.
     */
    virtual void
    deletePath(Writable *, Parameter<Operation::DELETE_PATH> const &) = 0;
    /** Delete an existing dataset.
     *
     * The operation should fail if m_handler->m_frontendAccess is
     * Access::READ_ONLY. The operation should pass if the Writable was not
     * marked written. The dataset should have the name parameters.name. This
     * name should not start or end with a slash ("/"). It may also contain the
     * current dataset ".". The dataset should not be accessible in physical
     * storage after the operation completes successfully. The Writables file
     * position should be set to an invalid position (i.e. the pointer should be
     * a nullptr). The Writable should be marked not written when the operation
     * completes successfully.
     */
    virtual void
    deleteDataset(Writable *, Parameter<Operation::DELETE_DATASET> const &) = 0;
    /** Delete an existing attribute.
     *
     * The operation should fail if m_handler->m_frontendAccess is
     * Access::READ_ONLY. The operation should pass if the Writable was not
     * marked written. The attribute should be associated with the Writable and
     * have the name parameters.name before deletion. The attribute should not
     * be accessible in physical storage after the operation completes
     * successfully.
     */
    virtual void
    deleteAttribute(Writable *, Parameter<Operation::DELETE_ATT> const &) = 0;
    /** Write a chunk of data into an existing dataset.
     *
     * The operation should fail if m_handler->m_frontendAccess is
     * Access::READ_ONLY. The dataset should be associated with the Writable.
     * The operation should fail if the dataset does not exist.
     * The operation should fail if the chunk extent parameters.extent is not
     * smaller or equals in every dimension. The operation should fail if chunk
     * positions parameters.offset+parameters.extent do not reside inside the
     * dataset. The dataset should match the dataype parameters.dtype. The data
     * parameters.data is a cast-to-void pointer to a flattened version of the
     * chunk data. It should be re-cast to the provided datatype. The chunk is
     * stored row-major. The region of the chunk should be written to physical
     * storage after the operation completes successfully.
     */
    virtual void
    writeDataset(Writable *, Parameter<Operation::WRITE_DATASET> const &) = 0;

    /** Get a view into a dataset buffer that can be filled by a user.
     *
     * The operation should fail if m_handler->m_frontendAccess is
     * Access::READ_ONLY. The dataset should be associated with the Writable.
     * The operation should fail if the dataset does not exist.
     * The operation should fail if the chunk extent parameters.extent is not
     * smaller or equals in every dimension. The operation should fail if chunk
     * positions parameters.offset+parameters.extent do not reside inside the
     * dataset. The dataset should match the dataype parameters.dtype. The
     * buffer should be stored as a cast-to-char pointer to a flattened version
     * of the backend buffer in parameters.out->ptr. The chunk is stored
     * row-major. The buffer's content should be written to storage not before
     * the next call to AbstractIOHandler::flush where
     * AbstractIOHandler::m_flushLevel == FlushLevel::InternalFlush. The precise
     * time of data consumption is defined by the backend:
     * * Data written to the returned buffer should be consumed not earlier than
     * the next call to AbstractIOHandler::flush where
     * AbstractIOHandler::m_flushLevel == FlushLevel::InternalFlush.
     * * Data should be consumed not later than the next Operation::ADVANCE task
     * where parameter.mode == AdvanceMode::ENDSTEP.
     *
     * This IOTask is optional and should either (1) not be implemented by a
     * backend at all or (2) be implemented as indicated above and set
     * parameters.out->backendManagedBuffer = true.
     */
    virtual void
    getBufferView(Writable *, Parameter<Operation::GET_BUFFER_VIEW> &parameters)
    {
        // default implementation: operation unsupported by backend
        parameters.out->backendManagedBuffer = false;
    }
    /** Create a single attribute and fill the value, possibly overwriting an
     * existing attribute.
     *
     * The operation should fail if m_handler->m_frontendAccess is
     * Access::READ_ONLY. The attribute should have the name parameters.name.
     * This name should not contain a slash ("/"). The attribute should be of
     * datatype parameters.dtype. Any existing attribute with the same name
     * should be overwritten. If possible, only the value should be changed if
     * the datatype stays the same. The attribute should be written to physical
     * storage after the operation completes successfully. All datatypes of
     * Datatype should be supported in a type-safe way.
     */
    virtual void
    writeAttribute(Writable *, Parameter<Operation::WRITE_ATT> const &) = 0;
    /** Read a chunk of data from an existing dataset.
     *
     * The dataset should be associated with the Writable.
     * The operation should fail if the dataset does not exist.
     * The operation should fail if the chunk extent parameters.extent is not
     * smaller or equals in every dimension. The operation should fail if chunk
     * positions parameters.offset+parameters.extent do not reside inside the
     * dataset. The dataset should match the dataype parameters.dtype. The data
     * parameters.data should be a cast-to-void pointer to a flattened version
     * of the chunk data. The chunk should be stored row-major. The region of
     * the chunk should be written to the location indicated by the pointer
     * after the operation completes successfully.
     */
    virtual void
    readDataset(Writable *, Parameter<Operation::READ_DATASET> &) = 0;
    /** Read the value of an existing attribute.
     *
     * The operation should fail if the Writable was not marked written.
     * The operation should fail if the attribute does not exist.
     * The attribute should be associated with the Writable and have the name
     * parameters.name. This name should not contain a slash ("/"). The
     * attribute datatype should be stored in the location indicated by the
     * pointer parameters.dtype. The attribute value should be stored as a
     * generic Variant::resource in the location indicated by the pointer
     * parameters.resource. All datatypes of Datatype should be supported in a
     * type-safe way.
     */
    virtual void
    readAttribute(Writable *, Parameter<Operation::READ_ATT> &) = 0;
    /** List all paths/sub-groups inside a group, non-recursively.
     *
     * The operation should fail if the Writable was not marked written.
     * The operation should fail if the Writable is not a group.
     * The list of group names should be stored in the location indicated by the
     * pointer parameters.paths.
     */
    virtual void listPaths(Writable *, Parameter<Operation::LIST_PATHS> &) = 0;
    /** List all datasets inside a group, non-recursively.
     *
     * The operation should fail if the Writable was not marked written.
     * The operation should fail if the Writable is not a group.
     * The list of dataset names should be stored in the location indicated by
     * the pointer parameters.datasets.
     */
    virtual void
    listDatasets(Writable *, Parameter<Operation::LIST_DATASETS> &) = 0;
    /** List all attributes associated with an object.
     *
     * The operation should fail if the Writable was not marked written.
     * The attribute should be associated with the Writable.
     * The list of attribute names should be stored in the location indicated by
     * the pointer parameters.attributes.
     */
    virtual void
    listAttributes(Writable *, Parameter<Operation::LIST_ATTS> &) = 0;

    AbstractIOHandler *m_handler;
}; // AbstractIOHandlerImpl
} // namespace openPMD
