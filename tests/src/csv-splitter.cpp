#include <algorithm>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "boost/program_options.hpp"
namespace po = boost::program_options;

void split_files(const vector<string> &inputs, const vector<string> &outputs,
                 const vector<int> &nums) {
  if (inputs.size() != outputs.size())
    throw domain_error("inputs and outputs must have the same size");
  if (inputs.size() != nums.size())
    throw domain_error("nums must have the same size as inputs");

  for (size_t m = 0; m < inputs.size(); m++) {
    ifstream input(inputs[m]);
    if (!input)
      throw runtime_error(inputs[m] + " could not be opened.");

    vector<ofstream> files(nums[m]);
    int n = 0;
    for (auto &file : files) {
      n++;
      file = ofstream(outputs[m] + "-" + to_string(n));
      if (!file)
        throw runtime_error(outputs[m] + "-" + to_string(n) +
                            " could not be opened.");
    }

    cout << "Splitting " << inputs[m] << " into " << nums[m]
         << " pieces with name " << outputs[m] << "...\n";

    string line;
    n = 0;
    while (getline(input, line))
      files[(n++) % nums[m]] << line << endl;
  }
}

template <class T> ostream &operator<<(ostream &os, const vector<T> &vec) {
  copy(std::begin(vec), std::end(vec), ostream_iterator<T>(os, " "));
  return os;
}

int main(int argc, char *argv[]) {
  vector<string> inputs;
  vector<string> outputs;
  vector<int> nums;

  po::options_description options("Options");
  options.add_options()("help,h", "prints the help message")(
      "input,i", po::value<vector<string>>(&inputs),
      "sets the input file(s) to split")("output,o",
                                         po::value<vector<string>>(&outputs),
                                         "sets the output file(s) to write")(
      "num,n", po::value<vector<int>>(&nums), "sets the number of files");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << options << '\n';
    return 0;
  } else if (!vm.count("input")) {
    cerr << "Input files are not set.\n";
    cout << options << '\n';
    return 1;
  } else if (!vm.count("output")) {
    cerr << "Output files are not set.\n";
    cout << options << '\n';
    return 2;
  } else if (!vm.count("num")) {
    cerr << "Numbers are not defined.\n";
    cout << options << '\n';
    return 3;
  }

  std::cout << "Input files: " << vm["input"].as<vector<string>>() << '\n';
  std::cout << "Output files: " << vm["output"].as<vector<string>>() << '\n';
  std::cout << "Number of files: " << vm["num"].as<vector<int>>() << '\n';

  split_files(inputs, outputs, nums);

  return 0;
}
