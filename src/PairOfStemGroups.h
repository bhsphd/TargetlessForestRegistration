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

#ifndef TLR_PAIROFSTEMGROUPS_H_
#define TLR_PAIROFSTEMGROUPS_H_

#include "StemMap.h"

namespace tlr
{

typedef std::vector<const Stem*> StemGroup;
// Helper functions declarations
void GetCentroid(const StemGroup group,
                 Eigen::Vector3d& centroid);
bool SortStemPointers(const Stem* stem1, const Stem* stem2);

class PairOfStemGroups
{
 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW // Fixes wierd memory crashes
  PairOfStemGroups(StemGroup& targetTriplet, StemGroup& sourceTriplet);
  ~PairOfStemGroups();
  const std::vector<double>& getRadiusSimilarity() const;
  const std::vector<double> getVerticeDifference() const;
  Eigen::Matrix4d computeBestTransform();
  Eigen::Matrix4d getBestTransform() const;
  const StemGroup getTargetGroup() const;
  const StemGroup getSourceGroup() const;
  void addFittingStem(const Stem* sourceStem, const Stem* targetStem);
  // To sort by likelihood, and if the transform is computed sort by MSE
  friend bool operator<(PairOfStemGroups& l, PairOfStemGroups& r);
  double getMeanSquareError() const;

 private:
  void sortStems();
  void updateRadiusSimilarity();
  double updateMeanSquareError();
  /* They are only triplet at first. We'll add other stems that fit the model later.
  These are a copy of the vector created by the Registration class. We need to copy them
  because differents pair will generate different models, which means in some case we we'll have
  to add a stem to a group, some case not. When we add a stem to a group we don't want to add it in every pair!

  TODO rendre ca plus clair
  */
  StemGroup targetGroup;
  StemGroup sourceGroup;
  double meanSquareError;
  /* They should be real but I put a complex type this way the
  compiler won't complain */
  std::vector<double> radiusSimilarity;
  /* This is sadly necessary because of alignement issues in Eigen. Declaring
  the transorm matrix as dynamically sized instead of 4x4 fixes bugs with matrix
  operations in MSVSC++ 2013 */
  Eigen::MatrixXd bestTransform;
  bool transformComputed;
};

} // namespace tlr
#endif
