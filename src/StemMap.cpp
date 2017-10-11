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

#include "StemMap.h"
#include <fstream>
#include <sstream>


namespace tlr
{

void Split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> Split(const std::string &s, char delim);

StemMap::StemMap()
{
  this->stems = std::vector<Stem, Eigen::aligned_allocator<Eigen::Vector4f>>();
  this->transMatrix = Eigen::Matrix4d::Identity(); // No transform applied yet
}

StemMap::StemMap(const StemMap& stemMap)
{
  this->stems =
    std::vector<Stem,Eigen::aligned_allocator<Eigen::Vector4f>>(stemMap.stems);
  this->transMatrix = Eigen::Matrix4d(stemMap.transMatrix);
}

StemMap::~StemMap()
{
}

void
StemMap::applyTransMatrix(const Eigen::Matrix4d& transMatrix)
{
  // Could gain significant speedup from parralelization
  for (auto& it : this->stems)
  {
    it.changeCoords(transMatrix);
  }

  this->transMatrix *= transMatrix; // We store the transformation
}

void
StemMap::removeStem(size_t indice)
{
  this->stems.erase(this->stems.begin() + indice);
}

void
StemMap::restoreOriginalCoords()
{
  // Simply apply the inverse transform!!
  this->applyTransMatrix(this->transMatrix.inverse());
  this->transMatrix = Eigen::Matrix4d::Identity();
}

void
StemMap::addStem(Stem &stem)
{
  this->stems.push_back(stem);
}

std::string
StemMap::strStemMap() const
{
  std::stringstream output;
  for (auto it : this->stems)
  {
    output << "Coords : " << it.getCoords()[0]
           << " " << it.getCoords()[1] << " " << it.getCoords()[2]
           << ", Radius : " << it.getRadius() << std::endl;
  }

  return output.str();
}

bool
StemMap::operator==(const StemMap &stemMap) const
{
  return stemMap.stems == this->stems &&
         stemMap.transMatrix == this->transMatrix;
}

const std::vector<Stem, Eigen::aligned_allocator<Eigen::Vector4f>>&
StemMap::getStems() const
{
  return this->stems;
}

void
StemMap::loadStemMapFile(std::string path, double minDiam)
{
  std::ifstream stemMapFile(path);
  std::string line;
  Stem tempStem;

  while (std::getline(stemMapFile, line))
  {
    std::vector<std::string> lineData = Split(line, ' ');
    if (std::stod(lineData[3]) > minDiam)
    {
      tempStem = Stem(std::stod(lineData[0]),
                      std::stod(lineData[1]),
                      std::stod(lineData[2]),
                      std::stod(lineData[3]));
      this->addStem(tempStem);
    }
  }
}

/*
Stack overflow code for splitting string
Used in StemMap::loadStemMapFile
*/
void
Split(const std::string &s, char delim, std::vector<std::string> &elems)
{
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
}


std::vector<std::string>
Split(const std::string &s, char delim)
{
  std::vector<std::string> elems;
  Split(s, delim, elems);
  return elems;
}
// End of stackoverflow code

} // namespace tlr
