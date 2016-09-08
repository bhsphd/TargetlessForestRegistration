#include "Registration.h"
#include <unordered_set>
#include <set>
#include <atomic>

namespace tlr
{
	
// Helper functions declaration
double GetMeanOfVector(const Eigen::Vector4d& coords);
std::vector<std::set<int>> NCombK(const int n, const int k);
bool ColinearityGreaterThanTol(StemTriplet& triplet);

// Getting ready for RANSAC, no heavy computation yet.
Registration::Registration(const StemMap& target, const StemMap& source,
						   double diamErrorTol, double RANSACtol) :
	target(target),
	source(source),
	diamErrorTol(diamErrorTol),
	RANSACtol(RANSACtol)
{
	std::cout << "Number of unmatched stems: " << this->removeLonelyStems() << std::endl;
	std::cout << "Number of stems in source: " << this->source.getStems().size() << std::endl;
	std::cout << "Number of stems in target: " << this->target.getStems().size() << std::endl;
	std::cout << "Generating triplets... " << std::endl;
	this->generateTriplets(this->source, this->threePermSource);
	this->generateTriplets(this->target, this->threePermTarget);
	this->generateAllEigenValues();
	//this->removeHighlyColinearTriplets(this->threePermSource);
	//this->removeHighlyColinearTriplets(this->threePermTarget);
	std::cout << "Generating pairs of triplets... " << std::endl;
	this->generatePairs();	
	std::cout << this->pairsOfStemTriplets.size() << " transforms to compute. " << std::endl;
	//this->sortPairsByLikelihood(); A voir la pertinence
}

void
Registration::computeBestTransform()
{
	if(this->pairsOfStemTriplets.size() == 0) return; // Nothing to compute

	std::cout << "Computing transformations... " << std::endl;

	#pragma omp parallel for
	for (size_t i = 0; i < this->pairsOfStemTriplets.size(); ++i)
	{
		this->pairsOfStemTriplets[i].computeBestTransform();
		this->RANSACtransform(this->pairsOfStemTriplets[i]);
	}

	std::cout << "Sorting results..." << std::endl;
	std::sort(this->pairsOfStemTriplets.begin(), this->pairsOfStemTriplets.end());
	
	std::cout << "====== Best transform ======" << std::endl
		<< this->pairsOfStemTriplets.begin()->getBestTransform() << std::endl
		<< "MSE : " << this->pairsOfStemTriplets.begin()->getMeanSquareError() << std::endl
		<< "Number of used stems : " << this->pairsOfStemTriplets.begin()->getTargetGroup().size() << std::endl
		<< "------ 3 first stems used for registration -----" << std::endl
		<< "Target : " << std::endl
		<< "Stem 1 : " << this->pairsOfStemTriplets.begin()->getTargetGroup()[0]->getCoords()
		<< std::endl << "Radius : " << this->pairsOfStemTriplets.begin()->getTargetGroup()[0]->getRadius() << std::endl
		<< "Stem 2 : " << this->pairsOfStemTriplets.begin()->getTargetGroup()[1]->getCoords()
                << std::endl << "Radius : " << this->pairsOfStemTriplets.begin()->getTargetGroup()[1]->getRadius() << std::endl
		<< "Stem 3 : " << this->pairsOfStemTriplets.begin()->getTargetGroup()[2]->getCoords()
                << std::endl << "Radius : " << this->pairsOfStemTriplets.begin()->getTargetGroup()[2]->getRadius() << std::endl
		<< "Source : " << std::endl
                << "Stem 1 : " << this->pairsOfStemTriplets.begin()->getSourceGroup()[0]->getCoords()
                << std::endl << "Radius : " << this->pairsOfStemTriplets.begin()->getSourceGroup()[0]->getRadius() << std::endl
                << "Stem 2 : " << this->pairsOfStemTriplets.begin()->getSourceGroup()[1]->getCoords()
                << std::endl << "Radius : " << this->pairsOfStemTriplets.begin()->getSourceGroup()[1]->getRadius() << std::endl
                << "Stem 3 : " << this->pairsOfStemTriplets.begin()->getSourceGroup()[2]->getCoords()
                << std::endl << "Radius : " << this->pairsOfStemTriplets.begin()->getSourceGroup()[2]->getRadius() << std::endl;
}

void
Registration::RANSACtransform(PairOfStemGroups& pair)
{	
	StemMap sourceCopy;
	bool keepGoing = true;

	while(keepGoing)
	{
		keepGoing = false;
		sourceCopy = StemMap(this->source);
		sourceCopy.applyTransMatrix(pair.getBestTransform());

		for(size_t i = 0; i < sourceCopy.getStems().size(); ++i)
		{
			for(size_t j = 0; j < this->target.getStems().size(); ++j)
			{
				if(!this->stemDistanceGreaterThanTol(sourceCopy.getStems()[i],
													 this->target.getStems()[j])
				&&
				!this->stemAlreadyInGroup(this->target.getStems()[j],
									pair.getTargetGroup())
				&&
				!this->relDiamErrorGreaterThanTol(this->target.getStems()[j],
												  this->source.getStems()[i]))
				{
					// We add the stem who was not transformed
					pair.addFittingStem(&this->source.getStems()[i],
										&this->target.getStems()[j]);
					keepGoing = true;
				}	
			}
		}	
		if(keepGoing) pair.computeBestTransform();
	}	
}

bool
Registration::stemAlreadyInGroup(const Stem& stem,
								 const std::vector<const Stem*> group) const
{
	for(const auto it : group)
	{
		if(stem.getCoords() == it->getCoords()) return true;
	}
	return false;
}

bool
Registration::stemDistanceGreaterThanTol(const Stem& stem1, const Stem& stem2) const
{
	Eigen::Vector4d stemError = stem1.getCoords()
		- stem2.getCoords();
	return stemError.norm() > this->RANSACtol;
}

bool
Registration::relDiamErrorGreaterThanTol(const Stem& stem1, const Stem& stem2) const
{
	return fabs(stem1.getRadius() - stem2.getRadius()) /
    	((stem1.getRadius() + stem2.getRadius())/2) < this->diamErrorTol;
}

Registration::~Registration()
{
}

unsigned int
Registration::removeLonelyStems()
{
	bool toBeRemoved;
	std::vector<size_t> indicesToRemove = {};
	unsigned int nRemoved = 0;
	size_t i = 0;
	for (auto& itSource : this->source.getStems())
	{
		toBeRemoved = true;
		for (auto& itTarget : this->target.getStems())
		{
			if (!this->relDiamErrorGreaterThanTol(
				itTarget, itSource
				))
			{
				toBeRemoved = false;
			}
		}
		if (toBeRemoved)
		{
			indicesToRemove.push_back(i);
			++nRemoved;
		}
		++i;
	}
	// If any stem has to be removed
	if (indicesToRemove.size() > 0)
	{
		// Remove the stems backward so the indices are not all shuffled up
		for (size_t j = indicesToRemove.size() - 1; j > 0; --j)
		{
			this->source.removeStem(indicesToRemove[j]);
		}
	}
	// Repeat for the target map
	indicesToRemove = {};
	i = 0;
	for (auto& itTarget : this->target.getStems())
	{
		toBeRemoved = true;
		for (auto& itSource : this->source.getStems())
		{
			if (!this->relDiamErrorGreaterThanTol(
				itSource, itTarget
				))
			{
				toBeRemoved = false; 
			}
		}
		if (toBeRemoved)
		{
			indicesToRemove.push_back(i);
			++nRemoved;
		}
		++i;
	}
	if (indicesToRemove.size() > 0)
	{
		// Remove the stems backward so the indices are not all shuffled up
		for (size_t j = indicesToRemove.size() - 1; j > 0; --j)
		{
			this->target.removeStem(indicesToRemove[j]);
		}
	}
	return nRemoved;
}

long long
Factorial(int n)
{
	long long result = 1;
	while (n>1) {
		result *= n--;
	}
	return result;
}


std::vector<std::set<int>>
ThreeCombK(const unsigned int k)
{
	std::vector<std::set<int>> result;
	std::set<int> comb;
		
	for (unsigned int i = 1; i < k - 1; ++i)
	{
		for (unsigned int j = i; j < k; ++j)
		{
			for (unsigned int s = j; s <= k; ++s)
			{
				comb.insert(i);
				comb.insert(j);
				comb.insert(s);
				// Only add combinaison if the numbers are unique
				if (comb.size() == 3)
					result.push_back(comb);
				comb = std::set<int>();
			}
		}
	}

	return result;
}

void
Registration::generateEigenValues(StemTriplet& triplet)
{
	// Can be coded much cleaner. Needs to be revisited.
	Eigen::Matrix3d covarianceMatrix;
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			covarianceMatrix(i, j) =
				 ((std::get<0>(triplet))[i]->getCoords()[0] - GetMeanOfVector((std::get<0>(triplet))[i]->getCoords()))
				*((std::get<0>(triplet))[j]->getCoords()[0] - GetMeanOfVector((std::get<0>(triplet))[j]->getCoords()))
				+((std::get<0>(triplet))[i]->getCoords()[1] - GetMeanOfVector((std::get<0>(triplet))[i]->getCoords()))
				*((std::get<0>(triplet))[j]->getCoords()[1] - GetMeanOfVector((std::get<0>(triplet))[j]->getCoords()))
				+((std::get<0>(triplet))[i]->getCoords()[2] - GetMeanOfVector((std::get<0>(triplet))[i]->getCoords()))
				*((std::get<0>(triplet))[j]->getCoords()[2] - GetMeanOfVector((std::get<0>(triplet))[j]->getCoords()));
		}
	}
	Eigen::Matrix3d::EigenvaluesReturnType eigenvalues = covarianceMatrix.eigenvalues();
	std::get<1>(triplet).push_back(eigenvalues(0));
	std::get<1>(triplet).push_back(eigenvalues(1));
	std::get<1>(triplet).push_back(eigenvalues(2));
}

void
Registration::generateAllEigenValues()
{
	for (auto& it : this->threePermSource)
	{
		this->generateEigenValues(it);
	}
	for (auto& it : this->threePermTarget)
	{
		this->generateEigenValues(it);
	}
}

// This is useful for computing the covariance matrix.
double
GetMeanOfVector(const Eigen::Vector4d& coords)
{
	return (coords[0] + coords[1] + coords[2]) / 3;
}

/* This function populate the stem triplets from both the target scan and the source scan.
   We use the nPerm function to determine all the possible combinations. */
void
Registration::generateTriplets(StemMap& stemMap, std::vector<StemTriplet>& threePerm)
{
	std::vector<std::set<int>> threePermN = ThreeCombK(stemMap.getStems().size());
	StemTriplet tempTriplet = StemTriplet();
	
	for (auto it : threePermN)
	{
		for (auto jt : it)
			std::get<0>(tempTriplet).push_back(&stemMap.getStems()[jt - 1]);
		threePerm.push_back(tempTriplet);
		tempTriplet = StemTriplet();
	}
}

// Population the pairOfStemsTriplets attributes with all possible pairs. Cleanup is done later.
void
Registration::generatePairs()
{
	#pragma omp parallel for
	for (size_t i = 0; i < this->threePermSource.size(); ++i)
	{
		for (size_t j = 0; j < this->threePermTarget.size(); ++j)
		{
			PairOfStemGroups tempPair(this->threePermTarget[j],
									  this->threePermSource[i]);

			if(!this->diametersNotCorresponding(tempPair)
			   && this->pairPositionsAreCorresponding(tempPair))
			{	
				#pragma omp critical
				{
					this->pairsOfStemTriplets.push_back(tempPair);
				}
			}
		}
	}
}

void
Registration::removeHighlyColinearTriplets(std::vector<StemTriplet>& triplets)
{
	triplets.erase(
		std::remove_if(
			triplets.begin(),
			triplets.end(),
			ColinearityGreaterThanTol),
		triplets.end());
}

// This removes of non-matching pair of triplets.
bool
Registration::diametersNotCorresponding(PairOfStemGroups& pair)
{
	for(auto& it : pair.getRadiusSimilarity())
	{
		if(this->diamErrorGreaterThanTol(it)) return true;
	}
	return false;
}

bool
Registration::pairPositionsAreCorresponding(PairOfStemGroups& pair)
{
	const std::vector<double> edgeDiffs = pair.getEdgeDifference();
	for(double diff : edgeDiffs)
	{
		if(diff > 2*this->RANSACtol) return false;
	}
	return true;
}

// This is used for the removal of non-matching pair of triplets (DiametersNotCorresponding).
bool
Registration::diamErrorGreaterThanTol(double error)
{
	return error > this->diamErrorTol;
}

/* This is used to determine if the triplet of stem is too linear, which makes
   it unfit for the rigid transform. */
bool
ColinearityGreaterThanTol(StemTriplet& triplet)
{
	return std::get<1>(triplet)[0].real() /
		(std::get<1>(triplet)[0].real() + std::get<1>(triplet)[1].real())
		> LINEARITY_TOL;
}

} // namespace tlr