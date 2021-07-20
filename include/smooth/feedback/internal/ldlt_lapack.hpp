
#include <Eigen/Core>

#include <lapacke.h>

namespace smooth::feedback::detail {

template<typename Scalar>
struct lapack_ldlt_fcn
{
  static constexpr auto value = &LAPACKE_ssysvx;
};

template<>
struct lapack_ldlt_fcn<double>
{
  static constexpr auto value = &LAPACKE_dsysvx;
};

/**
 * @brief Wrapper for LAPACKE xSYSVX routing for symmetric systems of equations.
 */
template<typename Scalar, Eigen::Index N>
  requires std::is_same_v<Scalar, float> || std::is_same_v<Scalar, double> class LDLTLapack
{
public:
  /**
   * @brief Factorize symmetric \f$ A \f$ to enable solving \f$ A x = b \f$.
   *
   * \p A is factorized as \f$ A = U D U^T \f$ where \f$ U \f$ is upper triangular
   * and \f$ D \f$ is block-diagonal.
   *
   * @param A symmetric matrix to factorize
   */
  template<typename Derived>
  inline LDLTLapack(const Eigen::MatrixBase<Derived> & A)
      : n_(A.cols()), A_(A), AF_(A.cols(), A.cols()), IPIV_(A.cols())
  {
    Eigen::Matrix<Scalar, N, 1> b(n_), x(n_);
    b.setZero();
    Scalar rcond, ferr, berr;

    info_ = (*lapack_ldlt_fcn<Scalar>::value)(LAPACK_COL_MAJOR,
      'N',           // FACT: factor incoming matrix
      'U',           // UPLO
      n_,            // N
      1,             // NRHS
      A_.data(),     // A
      n_,            // LDA
      AF_.data(),    // AF
      n_,            // LDAF
      IPIV_.data(),  // IPIV
      b.data(),      // B
      n_,            // LDB
      x.data(),      // X
      n_,            // LDX
      &rcond,
      &ferr,
      &berr);
  }

  /**
   * @brief Factorization status
   *
   * * 0: successful exit
   * * i > 0: input matrix is singular with D(i, i) = 0.
   */
  inline lapack_int info() const { return info_; }

  /**
   * @brief Solve linear symmetric system of equations.
   *
   * @param b right-hand side in \f$ A x = b \f$.
   */
  inline Eigen::Matrix<Scalar, N, 1> solve(const Eigen::Matrix<Scalar, N, 1> & b) const
  {
    Eigen::Matrix<Scalar, N, 1> x(n_);
    Scalar rcond, ferr, berr;

    (*lapack_ldlt_fcn<Scalar>::value)(LAPACK_COL_MAJOR,
      'F',                                     // FACT: factor incoming matrix
      'U',                                     // UPLO
      n_,                                      // N
      1,                                       // NRHS
      A_.data(),                               // A
      n_,                                      // LDA
      const_cast<Scalar *>(AF_.data()),        // AF
      n_,                                      // LDAF
      const_cast<lapack_int *>(IPIV_.data()),  // IPIV
      b.data(),                                // B
      n_,                                      // LDB
      x.data(),                                // X
      n_,                                      // LDX
      &rcond,
      &ferr,
      &berr);

    return x;
  }

private:
  lapack_int n_;
  Eigen::Matrix<Scalar, N, N> A_, AF_;
  Eigen::Matrix<lapack_int, N, 1> IPIV_;
  lapack_int info_;
};

}  // namespace smooth::feedback::detail