/***************************************************************************
 *   Copyright (C) 2017 by Jean-François Tremblay                          *
 *   jftremblay255@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef TLR_STEMMAP_H_
#define TLR_STEMMAP_H_

#include <Eigen/StdVector>
#include <string>
#include <iostream>
#include "Stem.h"

namespace tlr
{

class StemMap
{
 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  StemMap();
  StemMap(const StemMap& stemMap);
  ~StemMap();

  void loadStemMapFile(std::string path, double minDiam);
  void applyTransMatrix(const Eigen::Matrix4d& transMatrix);
  void addStem(Stem& stem);
  void restoreOriginalCoords();
  std::string strStemMap() const;
  bool operator==(const StemMap& stemMap) const;
  const std::vector<Stem, Eigen::aligned_allocator<Eigen::Vector4f>>& getStems() const;
  void removeStem(size_t indice);

 private:
  /*
  The aligned_allocator is necessary because of a "bug" in C++98.
  maybe compiling with C++14 or C++17 will fix it. Source :
  https://eigen.tuxfamily.org/dox/group__TopicStlContainers.html
  */
  std::vector<Stem,Eigen::aligned_allocator<Eigen::Vector4f>> stems;
  Eigen::Matrix4d transMatrix; // Transformation matrix since the original
};

} // namespace tlr
#endif
