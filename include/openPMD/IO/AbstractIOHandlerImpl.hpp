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
#pragma once

#include "openPMD/IO/IOTask.hpp"

#include <future>


namespace openPMD
{
class AbstractIOHandler;
class Writable;

class AbstractIOHandlerImpl
{
public:
  AbstractIOHandlerImpl(AbstractIOHandler*);
  virtual ~AbstractIOHandlerImpl();

  virtual std::future< void > flush();

  virtual void createFile(Writable*, Parameter< Operation::CREATE_FILE > const&) = 0;
  virtual void createPath(Writable*, Parameter< Operation::CREATE_PATH > const&) = 0;
  virtual void createDataset(Writable*, Parameter< Operation::CREATE_DATASET > const&) = 0;
  virtual void extendDataset(Writable*, Parameter< Operation::EXTEND_DATASET > const&) = 0;
  virtual void openFile(Writable*, Parameter< Operation::OPEN_FILE > const&) = 0;
  virtual void openPath(Writable*, Parameter< Operation::OPEN_PATH > const&) = 0;
  virtual void openDataset(Writable*, Parameter< Operation::OPEN_DATASET > &) = 0;
  virtual void deleteFile(Writable*, Parameter< Operation::DELETE_FILE > const&) = 0;
  virtual void deletePath(Writable*, Parameter< Operation::DELETE_PATH > const&) = 0;
  virtual void deleteDataset(Writable*, Parameter< Operation::DELETE_DATASET > const&) = 0;
  virtual void deleteAttribute(Writable*, Parameter< Operation::DELETE_ATT > const&) = 0;
  virtual void writeDataset(Writable*, Parameter< Operation::WRITE_DATASET > const&) = 0;
  virtual void writeAttribute(Writable*, Parameter< Operation::WRITE_ATT > const&) = 0;
  virtual void readDataset(Writable*, Parameter< Operation::READ_DATASET > &) = 0;
  virtual void readAttribute(Writable*, Parameter< Operation::READ_ATT > &) = 0;
  virtual void listPaths(Writable*, Parameter< Operation::LIST_PATHS > &) = 0;
  virtual void listDatasets(Writable*, Parameter< Operation::LIST_DATASETS > &) = 0;
  virtual void listAttributes(Writable*, Parameter< Operation::LIST_ATTS > &) = 0;

  AbstractIOHandler* m_handler;
};  //AbstractIOHandlerImpl
} // openPMD
