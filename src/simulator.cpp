#include <chrono>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>
using namespace std;

#include "boost/program_options.hpp"
namespace po = boost::program_options;

#include "polo/polo.hpp"
using namespace polo;

using index_t = int32_t;
using value_t = float;

int main(int argc, char *argv[]) {
  size_t id;
  string suffix;
  value_t threshold;
  unsigned int W;

  po::options_description options("Options");
  options.add_options()("help,h", "prints the help message")(
      "dataset-id,d", po::value<size_t>(&id),
      "sets the id of the dataset to load")("suffix,s",
                                            po::value<string>(&suffix),
                                            "suffix added to the dataset name")(
      "threshold,t", po::value<value_t>(&threshold)->default_value(1e-2),
      "sets the threshold in reporting nnz")(
      "nworkers,W",
      po::value<unsigned int>(&W)->default_value(
          thread::hardware_concurrency()),
      "sets the number of worker processes");

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

  vector<pair<string, bool>> datasets;
  string line;
  while (getline(dslist, line)) {
    istringstream ss(line);
    string dsname;
    bool dense;
    ss >> dsname >> boolalpha >> dense;
    datasets.push_back({dsname, dense});
  }

  if (!vm.count("dataset-id")) {
    cerr << "Dataset ID is not set.\n";
    cout << options << '\n';
    return 2;
  } else if (id >= datasets.size()) {
    cerr << "Dataset ID is set to " << id << ". Supported ID's are:\n";
    for (size_t idx = 0; idx < datasets.size(); idx++)
      cerr << "  - " << idx << ") " << datasets[idx].first << "("
           << (datasets[idx].second ? "dense" : "sparse") << ")\n";
    return 2;
  }

  if (!vm.count("suffix")) {
    cerr << "Suffix is not set.\n";
    cout << options << '\n';
    return 3;
  }

  const string dsfile = "data/" + datasets[id].first + "-0.bin";
  loss::data<value_t, index_t> dataset;
  try {
    dataset.load(dsfile, datasets[id].second);
  } catch (const exception &ex) {
    cerr << "Error occurred: " << ex.what() << '\n';
    return 4;
  }
  loss::logistic<value_t, index_t> logloss(dataset);

  const string logfile("results/" + datasets[id].first + "-" + suffix);
  ifstream infile(logfile + ".bin", ios_base::binary);
  if (!infile) {
    cerr << "Error occured: " << logfile << ".bin could not be opened.\n";
    return 5;
  }

  value_t lambda1, t;
  index_t n{0}, N, d;

  infile.read(reinterpret_cast<char *>(&lambda1), sizeof(value_t));
  infile.read(reinterpret_cast<char *>(&N), sizeof(index_t));
  infile.read(reinterpret_cast<char *>(&d), sizeof(index_t));

  cout << "Simulating from " << logfile << ".bin with\n";
  cout << "  - lambda1  : " << lambda1 << '\n';
  cout << "  - threshold: " << threshold << '\n';
  cout << "  - numlogs  : " << N << '\n';
  cout << "  - W        : " << W << '\n';
  auto tstart = chrono::high_resolution_clock::now();

  mutex input;
  vector<tuple<index_t, value_t, value_t, index_t>> traces(N);

  vector<thread> workers(W);
  cout << "Spawning " << W
       << " workers to simulate the experiment in parallel...\n";
  for (auto &worker : workers)
    worker = thread([&, d, lambda1]() {
      index_t nlocal, k;
      value_t t, fval;
      vector<value_t> x(d), g(d);
      while (true) {
        {
          lock_guard<mutex> lock(input);
          nlocal = n++;
          if (nlocal >= N)
            break;
          infile.read(reinterpret_cast<char *>(&k), sizeof(index_t));
          infile.read(reinterpret_cast<char *>(&t), sizeof(value_t));
          infile.read(reinterpret_cast<char *>(&x[0]), d * sizeof(value_t));
        }
        fval = logloss(&x[0], &g[0]);

        value_t absval, maxabsval{0};
        for (const value_t val : x) {
          absval = abs(val);
          if (absval > maxabsval)
            maxabsval = absval;
          fval += lambda1 * absval;
        }

        index_t nnz{0};
        for (const value_t val : x)
          if (abs(val) >= threshold * maxabsval)
            nnz++;

        cout << "k = " << k << ", t = " << t << ", fval = " << fval
             << ", nnz = " << nnz << '\n';

        traces[nlocal] = {k, t, fval, nnz};
      }
    });

  for (auto &worker : workers)
    worker.join();

  ofstream outfile(logfile + ".csv");

  cout << "Saving the traces to " << logfile << ".csv...\n";
  outfile << "k,t,fval,nnz\n";
  for (const auto &trace : traces)
    outfile << get<0>(trace) << ',' << get<1>(trace) << ',' << get<2>(trace)
            << ',' << get<3>(trace) << '\n';

  auto tend = chrono::high_resolution_clock::now();
  auto telapsed = chrono::duration_cast<chrono::seconds>(tend - tstart).count();
  auto hours = telapsed / 3600;
  auto minutes = (telapsed % 3600) / 60;
  auto seconds = (telapsed % 3600) % 60;
  cout << "Simulation took " << hours << ':' << minutes << ':' << seconds
       << ".\n";

  return 0;
}
