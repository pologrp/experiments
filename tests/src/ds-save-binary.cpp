#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
using namespace std;

#include "boost/program_options.hpp"
namespace po = boost::program_options;

#include "polo/polo.hpp"
using namespace polo;

template <class value_t, class index_t>
void printinfo(polo::loss::data<value_t, index_t> dataset, const index_t nrows,
               const index_t ncols) {
  cout << "The dataset has " << dataset.nsamples() << " samples, each having "
       << dataset.nfeatures() << " features. The dataset occupies "
       << dataset.size() / 1024 / 1024 << "MBs of space.\n";
  for (index_t row = 0; row < nrows; row++) {
    const auto rowvec = (*dataset.matrix()).getrow(row);
    const auto colind = (*dataset.matrix()).colindices(row);
    for (index_t col = 0; col < ncols && col < index_t(colind.size()); col++)
      cout << "A[" << row << ',' << colind[col]
           << "]: " << (*dataset.matrix())(row, colind[col]) << '\n';
  }
}

using index_t = int32_t;
using value_t = float;

int main(int argc, char *argv[]) {
  size_t id;
  string suffix;

  po::options_description options("Options");
  options.add_options()("help,h", "prints the help message")(
      "dataset-id,d", po::value<size_t>(&id),
      "sets the id of the dataset to load")(
      "suffix,s", po::value<string>(&suffix),
      "sets the suffix to append to the name");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << options << '\n';
    return 0;
  }

  ifstream dslist("data/datasets.lst");
  if (!dslist) {
    cerr << "Error occured: data/datasets.lst could not be opened.\n";
    return 1;
  }

  vector<tuple<string, bool, index_t, index_t>> datasets;
  string line;
  while (getline(dslist, line)) {
    istringstream ss(line);
    string dsname;
    bool dense;
    index_t nsamples, nfeatures;
    ss >> dsname >> boolalpha >> dense >> nsamples >> nfeatures;
    datasets.emplace_back(dsname, dense, nsamples, nfeatures);
  }

  if (!vm.count("dataset-id")) {
    cerr << "Dataset ID is not set.\n";
    cout << options << '\n';
    return 2;
  } else if (id >= datasets.size()) {
    cerr << "Dataset ID is set to " << id << ". Supported ID's are:\n";
    for (size_t idx = 0; idx < datasets.size(); idx++)
      cerr << "  - " << idx << ") " << get<0>(datasets[idx]) << "("
           << (get<1>(datasets[idx]) ? "dense" : "sparse") << ")\n";
    return 2;
  }
  const auto choice = datasets[id];

  if (!vm.count("suffix")) {
    cerr << "Suffix is not set.\n";
    cout << options << '\n';
    return 3;
  }

  const string dsname = "data/" + get<0>(choice) + "-" + suffix;
  ifstream dsfile(dsname);
  if (!dsfile) {
    cerr << "Error occurred: " << dsname << " could not be opened.\n";
    return 4;
  } else
    dsfile.close();

  cout << "Loading svm file from " << dsname << "...\n";
  polo::loss::data<value_t, index_t> dataset;
  if (get<1>(choice))
    dataset = utility::reader<value_t, index_t>::svm({dsname}, get<2>(choice),
                                                     get<3>(choice));
  else
    dataset = utility::reader<value_t, index_t>::svm({dsname});

  printinfo(dataset, 3, 5);

  cout << "Saving dataset to " << dsname << ".bin ...\n";
  dataset.save(dsname + ".bin");

  cout << "Loading the binary file to test...\n";
  dataset.load(dsname + ".bin", get<1>(choice));

  printinfo(dataset, 3, 5);

  value_t maxnorm{0};
  for (index_t idx = 0; idx < dataset.nsamples(); idx++) {
    const auto sample = (*dataset.matrix()).getrow(idx);
    value_t norm{0};
    for (const auto val : sample)
      norm += val * val;
    norm = sqrt(norm);
    if (norm >= maxnorm)
      maxnorm = norm;
  }
  cout << "Maximum 2-norm among the samples is " << maxnorm << ".\n";

  return 0;
}
