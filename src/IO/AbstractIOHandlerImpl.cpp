/* Copyright 2018 Fabian Koller
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
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerImpl.hpp"


namespace openPMD
{
AbstractIOHandlerImpl::AbstractIOHandlerImpl(AbstractIOHandler *handler)
    : m_handler{handler}
{ }

AbstractIOHandlerImpl::~AbstractIOHandlerImpl() = default;

std::future< void >
AbstractIOHandlerImpl::flush()
{
  while( !(*m_handler).m_work.empty() )
  {
    IOTask& i = (*m_handler).m_work.front();
    try
    {
      switch( i.operation )
      {
        using O = Operation;
        case O::CREATE_FILE:
          createFile(i.writable, *dynamic_cast< Parameter< Operation::CREATE_FILE >* >(i.parameter.get()));
          break;
        case O::CREATE_PATH:
          createPath(i.writable, *dynamic_cast< Parameter< O::CREATE_PATH >* >(i.parameter.get()));
          break;
        case O::CREATE_DATASET:
          createDataset(i.writable, *dynamic_cast< Parameter< O::CREATE_DATASET >* >(i.parameter.get()));
          break;
        case O::EXTEND_DATASET:
          extendDataset(i.writable, *dynamic_cast< Parameter< O::EXTEND_DATASET >* >(i.parameter.get()));
          break;
        case O::OPEN_FILE:
          openFile(i.writable, *dynamic_cast< Parameter< O::OPEN_FILE >* >(i.parameter.get()));
          break;
        case O::OPEN_PATH:
          openPath(i.writable, *dynamic_cast< Parameter< O::OPEN_PATH >* >(i.parameter.get()));
          break;
        case O::OPEN_DATASET:
          openDataset(i.writable, *dynamic_cast< Parameter< O::OPEN_DATASET >* >(i.parameter.get()));
          break;
        case O::DELETE_FILE:
          deleteFile(i.writable, *dynamic_cast< Parameter< O::DELETE_FILE >* >(i.parameter.get()));
          break;
        case O::DELETE_PATH:
          deletePath(i.writable, *dynamic_cast< Parameter< O::DELETE_PATH >* >(i.parameter.get()));
          break;
        case O::DELETE_DATASET:
          deleteDataset(i.writable, *dynamic_cast< Parameter< O::DELETE_DATASET >* >(i.parameter.get()));
          break;
        case O::DELETE_ATT:
          deleteAttribute(i.writable, *dynamic_cast< Parameter< O::DELETE_ATT >* >(i.parameter.get()));
          break;
        case O::WRITE_DATASET:
          writeDataset(i.writable, *dynamic_cast< Parameter< O::WRITE_DATASET >* >(i.parameter.get()));
          break;
        case O::WRITE_ATT:
          writeAttribute(i.writable, *dynamic_cast< Parameter< O::WRITE_ATT >* >(i.parameter.get()));
          break;
        case O::READ_DATASET:
          readDataset(i.writable, *dynamic_cast< Parameter< O::READ_DATASET >* >(i.parameter.get()));
          break;
        case O::READ_ATT:
          readAttribute(i.writable, *dynamic_cast< Parameter< O::READ_ATT >* >(i.parameter.get()));
          break;
        case O::LIST_PATHS:
          listPaths(i.writable, *dynamic_cast< Parameter< O::LIST_PATHS >* >(i.parameter.get()));
          break;
        case O::LIST_DATASETS:
          listDatasets(i.writable, *dynamic_cast< Parameter< O::LIST_DATASETS >* >(i.parameter.get()));
          break;
        case O::LIST_ATTS:
          listAttributes(i.writable, *dynamic_cast< Parameter< O::LIST_ATTS >* >(i.parameter.get()));
          break;
      }
    } catch (unsupported_data_error&)
    {
      (*m_handler).m_work.pop();
      throw;
    }
    (*m_handler).m_work.pop();
  }
  return std::future< void >();
}
} // openPMD
