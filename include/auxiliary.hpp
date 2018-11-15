#ifndef AUXILIARY_HPP_
#define AUXILIARY_HPP_

template <class value_t>
value_t dist(const vector<value_t> &x, const vector<value_t> &y) {
  value_t d{0};
  for (size_t idx = 0; idx < x.size(); idx++)
    d += (x[idx] - y[idx]) * (x[idx] - y[idx]);
  return d;
}

template <class value_t, class index_t>
struct customlogger : utility::logger::decision<value_t, index_t> {
  template <class InputIt1, class InputIt2>
  void operator()(const index_t k, const value_t fval, InputIt1 xbegin,
                  InputIt1 xend, InputIt2 gbegin) {
    if ((k == 1) | (k % 100 == 0)) {
      cout << "Logging at iteration k = " << k << ".\n";
      utility::logger::decision<value_t, index_t>::operator()(k, fval, xbegin,
                                                              xend, gbegin);
    }
  }
};

template <class value_t, class index_t>
void printinfo(loss::data<value_t, index_t> dataset, const index_t nrows,
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

#endif
