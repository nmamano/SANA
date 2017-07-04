#include "Database.hpp"
#include "utils/fileno.hpp"

using namespace std;

Database::Database()
	: Database(5)
{}

Database::Database(ullint k)
	: k_(k)
{
	/**Just reading the data from canon_map, canon_list, and orbit_map**/
	ullint m, decimal;
	string permutation;
	ifstream fcanon_map("data/canon_map"+to_string(k)+".txt"), forbit_map("data/orbit_map"+to_string(k)+".txt");
	ifstream fcanon_list("data/canon_list"+to_string(k)+".txt");
	databaseDir = "Database"+to_string(k)+"/";

	//reading canon_map
	graphette x;
	while(fcanon_map >> decimal >> permutation){
		x.canonicalDecimal = decimal;
		x.canonicalPermutation = permutation;
		g.push_back(x);
	}
	fcanon_map.close();
	//reading canon_list and orbit_map
	fcanon_list >> m; //reading the number of canonical graphettes
	forbit_map >> numOrbitId_; //reading the number of orbit ids
	for(ullint i = 0; i < m; i++){
		fcanon_list >> decimal;
		canonicalGraphette.push_back(decimal);
		//reading orbit ids for the canonical graphette decimal
		vector<ullint> ids(k);
		for(ullint j = 0; j < k; j++){
			forbit_map >> ids[j];
		}
		orbitId_.push_back(ids);
	}
	fcanon_list.close();
	fcanon_map.close();
	forbit_map.close();
}

#define MAX_FILES 1024 // ensure this is actually big enough!
void Database::addGraph(string filename, ullint numSamples){
	int firstfd=-1;
	//Prepocessing the input filename
	ifstream fgraph(filename);
	string graphName = filename;
	while(graphName.find("/") != string::npos)
		graphName = graphName.substr(graphName.find("/") + 1);
	//reading the graph from an edge list file
	vector<pair<ullint, ullint>> edgelist;
	ullint m, n;
	while(fgraph >> m >> n){
		edgelist.push_back(make_pair(m, n));
	}
	fgraph.close();
	Graph graph(edgelist);
	//orbit signature for each node
	vector<vector<bool>> orbitSignature(graph.numNodes(), vector<bool>(numOrbitId_, false));//each row for each node
	vector<bool> isOpen(numOrbitId_, false);
	vector<ofstream> forbitId(numOrbitId_);
	vector<int> orbit2fd(numOrbitId_, -1);
	vector<int> fd2orbit(MAX_FILES, -1);
	vector<int> fdCount(MAX_FILES, 0);
	ullint indicator = numSamples / 10;
	for(ullint i = 0; i < numSamples; i++){
		if(i % indicator == 0)
			cout << (i / indicator) * 10 << "% complete\n";
		auto edge = xrand(0, edgelist.size());
		Graphette* x = graph.sampleGraphette(k_, edgelist[edge].first, edgelist[edge].second);
		Graphette* y = this->getCanonicalGraphette(x);
		delete x;
		//save orbit ids in respective directories
		for(ullint j = 0; j < k_; j++){
			auto z = y->decimalNumber();
			ullint l = lower_bound(canonicalGraphette.begin(), canonicalGraphette.end(), z) - canonicalGraphette.begin();
			ullint id = orbitId_[l][j];
			if(!isOpen[id]) {
			    forbitId[id].open(databaseDir+to_string(id)+"/"+graphName, ios_base::app);
			    if(forbitId[id].fail()) { // time to close some of them
				int i, biggest, numClosed=0;
				// Find the biggest one
				biggest=firstfd;
				assert(firstfd>0);
				for(i=firstfd+1;i<MAX_FILES;i++)
				    if(fdCount[i] > fdCount[biggest]) biggest=i;
				// cerr << "most frequent orbit is " << fd2orbit[biggest] << " with count "<< fdCount[biggest];
				biggest = fdCount[biggest];
				// Close those that are not used much
				for(i=firstfd;i<MAX_FILES;i++){
				    int orbit = fd2orbit[i];
				    if(fdCount[i] < biggest/8 && orbit > 0) {
					assert(orbit>=0 && orbit < numOrbitId_);
					assert(orbit2fd[orbit]>0 && orbit2fd[orbit]<MAX_FILES);
					assert(isOpen[orbit]);
					forbitId[orbit].close();
					++numClosed;
					isOpen[orbit]=false;
					orbit2fd[orbit]=-1;
					fd2orbit[i]=-1;
				    }
				    fdCount[i] = 0; // reset for LRU
				}
				// Try again
				forbitId[id].open(databaseDir+to_string(id)+"/"+graphName, ios_base::app);
				if(forbitId[id].fail()) {
				    cerr << "really and truly can't open new file\n";
				    exit(1);
				} else {
				    // cerr << "; numClosed was "<<numClosed<<endl;
				}
			    }
			    orbit2fd[id]=fileno(forbitId[id]);
			    fd2orbit[orbit2fd[id]]=id;
			    isOpen[id] = true;
			    if(firstfd<0){
				firstfd=orbit2fd[id];
				// cerr << "firstfd is "<<firstfd<<endl;}
			    }
			}
			++fdCount[orbit2fd[id]];
			forbitId[id] << y->label(j) << " ";
			for(auto orbit: y->labels()){
				if(orbit != y->label(j))
					forbitId[id] << orbit << " ";
			}
			forbitId[id] << endl;
			orbitSignature[y->label(j)][id]= true;
		}
		delete y;
	}
	cout << "100% complete\n";
	for(ullint i = 0; i < numOrbitId_; i++) 
		if(isOpen[i]) forbitId[i].close();
	
	//save orbit signatures as a matrix
	ofstream fout(databaseDir + graphName + "nodeOrbitMembershipBitVector.txt");
	fout << "k = " << k_ << " numSamples = " << numSamples << endl;
	for(auto node: orbitSignature){
		for(auto id: node) fout << id;
		fout << endl;
	}
	fout.close();
}

Graphette* Database::getCanonicalGraphette(Graphette* x){
	ullint num = x->decimalNumber();
	Graphette* y = new Graphette(k_, g[num].canonicalDecimal);
	for(ullint i = 0; i < k_; i++){
		y->setLabel(g[num].canonicalPermutation[i] - '0', x->label(i));
	}
	return y;
}
