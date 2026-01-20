#include <bits/stdc++.h>
#include <pthread.h>
#include <cblas.h>
using namespace std;

static const int MAXN = 2048;
static const int BS   = 64;
static const double EPS = 1e-9;

/* ================= TIMING ================= */
double now() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ================= THREAD DATA ================= */
struct ThreadData {
    int tid, threads, N;
    double *A, *B, *BT, *C;
};

/* ================= MASTER MATRIX ================= */
vector<double> MASTER;

/* ================= UTIL ================= */
void init_master() {
    MASTER.resize(MAXN * MAXN);
    srand(0);
    for (auto &x : MASTER)
        x = rand() / (double)RAND_MAX;
}

void extract_submatrix(double *dst, int N) {
    for (int i = 0; i < N; i++)
        memcpy(&dst[i*N], &MASTER[i*MAXN], sizeof(double)*N);
}

void zero(double *M, int N) {
    memset(M, 0, sizeof(double)*N*N);
}

void transpose(double *B, double *BT, int N) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            BT[j*N + i] = B[i*N + j];
}

double max_abs_diff(double *A, double *B, int N) {
    double err = 0.0;
    for (int i = 0; i < N*N; i++)
        err = max(err, fabs(A[i] - B[i]));
    return err;
}

/* ================= BLAS REFERENCE ================= */
void blas_reference(double *A, double *B, double *C, int N) {
    cblas_dgemm(
        CblasRowMajor, CblasNoTrans, CblasNoTrans,
        N, N, N,
        1.0, A, N,
        B, N,
        0.0, C, N
    );
}

/* =====================================================
   1. IJK
   ===================================================== */
void* mm_ijk(void *arg) {
    auto *d = (ThreadData*)arg;
    int r0 = d->tid * d->N / d->threads;
    int r1 = (d->tid + 1) * d->N / d->threads;

    for (int i = r0; i < r1; i++)
        for (int j = 0; j < d->N; j++)
            for (int k = 0; k < d->N; k++)
                d->C[i*d->N+j] += d->A[i*d->N+k] * d->B[k*d->N+j];
    return nullptr;
}

/* =====================================================
   2. Transposed B
   ===================================================== */
void* mm_transposed(void *arg) {
    auto *d = (ThreadData*)arg;
    int r0 = d->tid * d->N / d->threads;
    int r1 = (d->tid + 1) * d->N / d->threads;

    for (int i = r0; i < r1; i++)
        for (int j = 0; j < d->N; j++) {
            double sum = 0;
            for (int k = 0; k < d->N; k++)
                sum += d->A[i*d->N+k] * d->BT[j*d->N+k];
            d->C[i*d->N+j] = sum;
        }
    return nullptr;
}

/* =====================================================
   3. IKJ
   ===================================================== */
void* mm_ikj(void *arg) {
    auto *d = (ThreadData*)arg;
    int r0 = d->tid * d->N / d->threads;
    int r1 = (d->tid + 1) * d->N / d->threads;

    for (int i = r0; i < r1; i++)
        for (int k = 0; k < d->N; k++) {
            double aik = d->A[i*d->N+k];
            for (int j = 0; j < d->N; j++)
                d->C[i*d->N+j] += aik * d->B[k*d->N+j];
        }
    return nullptr;
}

/* =====================================================
   4. Blocked (row parallel)
   ===================================================== */
void* mm_blocked(void *arg) {
    auto *d = (ThreadData*)arg;
    int r0 = d->tid * d->N / d->threads;
    int r1 = (d->tid + 1) * d->N / d->threads;

    for (int ii = r0; ii < r1; ii += BS)
        for (int kk = 0; kk < d->N; kk += BS)
            for (int jj = 0; jj < d->N; jj += BS)
                for (int i = ii; i < min(ii+BS, r1); i++)
                    for (int k = kk; k < min(kk+BS, d->N); k++) {
                        double aik = d->A[i*d->N+k];
                        for (int j = jj; j < min(jj+BS, d->N); j++)
                            d->C[i*d->N+j] += aik * d->B[k*d->N+j];
                    }
    return nullptr;
}

/* =====================================================
   5. Blocked Parallel (block-row cyclic)
   ===================================================== */
void* mm_blocked_parallel(void *arg) {
    auto *d = (ThreadData*)arg;
    int blocks = (d->N + BS - 1) / BS;

    for (int bi = d->tid; bi < blocks; bi += d->threads) {
        int ii = bi * BS;
        for (int kk = 0; kk < d->N; kk += BS)
            for (int jj = 0; jj < d->N; jj += BS)
                for (int i = ii; i < min(ii+BS, d->N); i++)
                    for (int k = kk; k < min(kk+BS, d->N); k++) {
                        double aik = d->A[i*d->N+k];
                        for (int j = jj; j < min(jj+BS, d->N); j++)
                            d->C[i*d->N+j] += aik * d->B[k*d->N+j];
                    }
    }
    return nullptr;
}

/* =====================================================
   6. 2D Tiled Parallel (NEW)
   ===================================================== */
void* mm_2d_tiled_parallel(void *arg) {
    auto *d = (ThreadData*)arg;
    int tiles = (d->N + BS - 1) / BS;

    for (int tile = d->tid; tile < tiles*tiles; tile += d->threads) {
        int ii = (tile / tiles) * BS;
        int jj = (tile % tiles) * BS;

        for (int kk = 0; kk < d->N; kk += BS)
            for (int i = ii; i < min(ii+BS, d->N); i++)
                for (int k = kk; k < min(kk+BS, d->N); k++) {
                    double aik = d->A[i*d->N+k];
                    for (int j = jj; j < min(jj+BS, d->N); j++)
                        d->C[i*d->N+j] += aik * d->B[k*d->N+j];
                }
    }
    return nullptr;
}

/* ================= RUNNER ================= */
void run(const string &name, void* (*fn)(void*),
         double *A, double *B, double *BT,
         double *C, double *Cref,
         int N, int threads, ofstream &out)
{
    vector<pthread_t> th(threads);
    vector<ThreadData> td(threads);
    zero(C, N);

    double t0 = now();
    for (int i = 0; i < threads; i++) {
        td[i] = {i, threads, N, A, B, BT, C};
        pthread_create(&th[i], nullptr, fn, &td[i]);
    }
    for (auto &t : th) pthread_join(t, nullptr);
    double t1 = now();

    double err = max_abs_diff(C, Cref, N);
    if (err > EPS)
        cout << "ERROR in " << name << " N=" << N << " err=" << err << endl;

    out << name << "," << N << "," << threads << "," << (t1 - t0) << "\n";
}

/* ================= MAIN ================= */
int main() {
    init_master();

    ofstream out("results.csv");
    out << "method,N,threads,time\n";

    vector<int> sizes   = {256, 512, 1024, 2048};
    vector<int> threads = {1, 2, 4, 8, 16};

    for (int N : sizes) {
        double *A = new double[N*N];
        double *B = new double[N*N];
        double *BT = new double[N*N];
        double *C = new double[N*N];
        double *Cref = new double[N*N];

        extract_submatrix(A, N);
        extract_submatrix(B, N);
        transpose(B, BT, N);

        zero(Cref, N);
        blas_reference(A, B, Cref, N);   // correctness reference

        for (int t : threads) {
            cout<<"Matmul for size "<<N<<" Threads "<<t<<"method ijk started "<<endl;
            run("ijk", mm_ijk, A, B, BT, C, Cref, N, t, out);
            cout<<"Matmul for size "<<N<<" Threads "<<t<<"method ijk end "<<endl;
            cout<<"Matmul for size "<<N<<" Threads "<<t<<"method transposed started "<<endl;
            run("transposed", mm_transposed, A, B, BT, C, Cref, N, t, out);
            cout<<"Matmul for size "<<N<<" Threads "<<t<<"method transposed end "<<endl;
            cout<<"Matmul for size "<<N<<" Threads "<<t<<"method ikj started "<<endl;
            run("ikj", mm_ikj, A, B, BT, C, Cref, N, t, out);
            cout<<"Matmul for size "<<N<<" Threads "<<t<<"method ikj end "<<endl;
            cout<<"Matmul for size "<<N<<" Threads "<<t<<"method blocked started "<<endl;
            run("blocked", mm_blocked, A, B, BT, C, Cref, N, t, out);
            cout<<"Matmul for size "<<N<<" Threads "<<t<<"method blocked end "<<endl;
            cout<<"Matmul for size "<<N<<" Threads "<<t<<"method blocked_parllel started "<<endl;
            run("blocked_parallel", mm_blocked_parallel, A, B, BT, C, Cref, N, t, out);
            cout<<"Matmul for size "<<N<<" Threads "<<t<<"method blocked_parllel end "<<endl;
            cout<<"Matmul for size "<<N<<" Threads "<<t<<"method 2d_tiled started "<<endl;
            run("2d_tiled_parallel", mm_2d_tiled_parallel, A, B, BT, C, Cref, N, t, out);
            cout<<"Matmul for size "<<N<<" Threads "<<t<<"method 2d_tiled end "<<endl;
        }

        delete[] A; delete[] B; delete[] BT; delete[] C; delete[] Cref;
    }

    out.close();
    cout << "DONE. Results written to results.csv\n";
}
