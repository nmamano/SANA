#include "ParetoFront.hpp"

bool ParetoFront::isDominating(vector<double> &scores)
{
    for(unsigned int i = 0; i < numberOfMeasures; i++)
        if(scores[i] > paretoFront[i].begin()->first)
            return true;
    return false;
}

char ParetoFront::whoDominates(vector<double> &newScores, vector<double> &otherScores)
{
    bool scoreA = false, scoreB = false;
    for(unsigned int i = 0; i < numberOfMeasures; i++) {
        if(newScores[i] > otherScores[i])
            scoreA = true;
        else
            scoreB = true;
        if(scoreA && scoreB)
            return 0;
    }
    if(scoreA)
        return 1;
    return 2;
}

bool ParetoFront::initialPass(vector<double> &newScores)
{
    char twoCounter = 0;
    for(unsigned int i = 0; i < numberOfMeasures; i++){
        singleValueIterator iter = prev(paretoFront[i].end());
        if(iter->first < newScores[i])
            twoCounter++;
    }
    if(twoCounter > 2)
        return true;
    return false;
}

alignmentPtr ParetoFront::removeAlignment(alignmentPtr alignmentPosition, vector<double> &alignmentScores)
{
    for(unsigned int i = 0; i < numberOfMeasures; i++) {
        multiValueIterator iterM = paretoFront[i].equal_range(alignmentScores[i]);
        for(singleValueIterator iterS = iterM.first; iterS != iterM.second; iterS++) {
            if(iterS->second == alignmentPosition) {
                paretoFront[i].erase(iterS);
                break;
            }
        }
    }
    findScoresByAlignment.erase(alignmentPosition);
    return alignmentPosition;
}

alignmentPtr ParetoFront::removeRandom()
{
    bool test = true;
    singleValueIterator iter;
    do {
        unsigned int iterate = rand() % (capacity - 1) + 1;
        test = false;
        iter = paretoFront[0].begin();
        advance(iter, iterate);
        for(unsigned int i = 1; i < numberOfMeasures; i++) {
            if(iter == paretoFront[i].begin()) {
                test = true;
                break;
            }
        }
    } while(test);
    return removeAlignment(iter->second, findScoresByAlignment[iter->second]);
}

vector<alignmentPtr> ParetoFront::emptyVector()
{
    return vector<alignmentPtr>(0);
}

vector<alignmentPtr> ParetoFront::removeNewlyDominiated(singleValueIterator &iterIN, unsigned int i, vector<double>& newScores)
{
    singleValueIterator iter = iterIN;
    vector<alignmentPtr> dominatedAlignments(0);
    while(prev(iter)->first == iter->first)
        iter--;
    while(iter != paretoFront[i].end()) {
        if(iter == iterIN) {
            iter++;
            continue;
        }
        vector<double> alignmentScores = findScoresByAlignment[iter->second];
        if(whoDominates(newScores, alignmentScores) == 1) {
            for(unsigned int j = 0; j < numberOfMeasures; j++) {
                if(j == i)
                    continue;
                singleValueIterator iterS = paretoFront[j].lower_bound(alignmentScores[j]);
                while(iterS != paretoFront[j].end()) {
                    if(iterS->second == iter->second) {
                        iterS = paretoFront[j].erase(iterS);
                        break;
                    }
                    else
                        iterS++;
                }
            }
            dominatedAlignments.push_back(iter->second);
            findScoresByAlignment.erase(iter->second);
            iter = paretoFront[i].erase(iter);
            currentSize--;
        }
        else
            iter++;
    }
    return dominatedAlignments;
}

vector<alignmentPtr> ParetoFront::tryToInsertAlignmentScore(alignmentPtr algmtPtr, vector<double> &newScores)
{
    double minDistanceFromBeginning = 0;
    double minDistanceFromEnd = (unsigned int)(0-1);
    unsigned int minDistanceIndexFromBeginning = 0;
    unsigned int minDistanceIndexFromEnd = 0;
    vector<singleValueIterator> iterators(numberOfMeasures);
    bool decisionToDelete = false;
    for(unsigned int i = 0; i < numberOfMeasures; i++) {
        iterators[i] = paretoFront[i].insert(pair<double, alignmentPtr>(newScores[i], algmtPtr));
        double distance = iterators[i]->first / (paretoFront[i].begin()->first - prev(paretoFront[i].end())->first);
        if(distance > minDistanceFromBeginning) {
            minDistanceFromBeginning = distance;
            minDistanceIndexFromBeginning = i;
        }
        if(distance < minDistanceFromEnd) {
            minDistanceFromEnd = distance;
            minDistanceIndexFromEnd = i;
        }
    }
    for(singleValueIterator iter = paretoFront[minDistanceIndexFromBeginning].begin(); iter != paretoFront[minDistanceIndexFromBeginning].end(); iter++)
    {
        if(iter->first <= iterators[minDistanceIndexFromBeginning]->first)
            break;
        if(iter == iterators[minDistanceIndexFromBeginning])
            continue;
        vector<double> alignmentScores = findScoresByAlignment[iter->second];
        if(whoDominates(newScores, alignmentScores) == 2) {
            decisionToDelete = true;
            break;
        }
    }
    if(decisionToDelete == true){
        for(unsigned int i = 0; i < numberOfMeasures; i++)
            paretoFront[i].erase(iterators[i]);
        return emptyVector();
    }
    findScoresByAlignment.insert(pair<alignmentPtr, vector<double>>(algmtPtr, newScores));
    vector<alignmentPtr> toReturn = removeNewlyDominiated(iterators[minDistanceIndexFromEnd], minDistanceIndexFromEnd, newScores);
    if(currentSize < capacity)
        currentSize++;
    else
        toReturn.push_back(removeRandom());
    if(toReturn.size() == 0)
        toReturn.push_back(NULL);
    return toReturn;
}

vector<alignmentPtr> ParetoFront::insertDominatingAlignmentScore(alignmentPtr algmtPtr, vector<double> &newScores)
{
    double minDistanceFromEnd = (unsigned int)(0-1);
    unsigned int minDistanceIndexFromEnd = 0;
    vector<singleValueIterator> iterators(numberOfMeasures);
    for(unsigned int i = 0; i < numberOfMeasures; i++) {
        iterators[i] = paretoFront[i].insert(pair<double, alignmentPtr>(newScores[i], algmtPtr));
        double distance = iterators[i]->first / (paretoFront[i].begin()->first - prev(paretoFront[i].end())->first);
        if(distance < minDistanceFromEnd) {
            minDistanceFromEnd = distance;
            minDistanceIndexFromEnd = i;
        }
    }
    findScoresByAlignment[iterators[0]->second] = vector<double>(newScores);
    vector<alignmentPtr> toReturn = removeNewlyDominiated(iterators[minDistanceIndexFromEnd], minDistanceIndexFromEnd, newScores);
    if(currentSize < capacity)
        currentSize++;
    else
        toReturn.push_back(removeRandom());
    if(toReturn.size() == 0)
        toReturn.push_back(NULL);
    return toReturn;
}

vector<alignmentPtr> ParetoFront::addAlignmentScores(alignmentPtr algmtPtr, vector<double> &newScores)
{
    if(isDominating(newScores))
        return insertDominatingAlignmentScore(algmtPtr, newScores);
    else if(initialPass(newScores))
        return tryToInsertAlignmentScore(algmtPtr, newScores);
    return emptyVector();
}

ostream& ParetoFront::printAlignmentScores(ostream &os)
{
    unsigned int i = 0;
    os << findScoresByAlignment.size() << ' ' << paretoFront[0].size() << ' ' << currentSize << '\n';
    for(auto iter = findScoresByAlignment.begin(); iter != findScoresByAlignment.end(); iter++)
    {
        for(unsigned int j = 0; j < numberOfMeasures; j++) {
            os << (iter->second)[j];
            if(j < numberOfMeasures - 1)
                os << " ";
        }
        i++;
        if(next(iter) != findScoresByAlignment.end())
            os << "\n";
    }
    return os;
}
