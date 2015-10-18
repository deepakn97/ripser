#include <vector>
#include <iostream>
#include <cassert>
#include <queue>
#include <unordered_map>
#include "prettyprint.hpp"
//#include <boost/numeric/ublas/symmetric.hpp>



class binomial_coeff_table {
    std::vector<std::vector<int> > B;
    int n_max, k_max;
    
public:
    binomial_coeff_table(int n, int k) {
        n_max = n;
        k_max = k;
    
        B.resize(n + 1);
        for( int i = 0; i <= n; i++ ) {
            B[i].resize(k + 1);
            for ( int j = 0; j <= std::min(i, k); j++ )
            {
                if (j == 0 || j == i)
                    B[i][j] = 1;

                else
                    B[i][j] = B[i-1][j-1] + B[i-1][j];
            }
        }
    }
    
    int operator()(int n, int k) const {
//        std::cout << "B(" << n << "," << k << ")\n";

        return B[n][k];
    }
};



template<typename OutputIterator>
OutputIterator get_simplex_vertices( int idx, int dim, int n, const binomial_coeff_table& binomial_coeff, OutputIterator out  )
{
    --n;
    
    for( int k = dim + 1; k > 0; --k ) {
        while( binomial_coeff( n , k ) > idx ) {
            --n;
        }
        *out++ = n;
        
        idx -= binomial_coeff( n , k );

    }

    return out;
}

template <typename DistanceMatrix>
class rips_filtration_comparator {
public:
    const DistanceMatrix& dist;
    
    const int dim;

private:
    std::vector<int> vertices;
    
    typedef decltype(dist(0,0)) dist_t;
    
    bool reverse;
    
    const binomial_coeff_table& binomial_coeff;
    
public:
    rips_filtration_comparator(
    const DistanceMatrix& _dist,
    const int _dim,
    const binomial_coeff_table& _binomial_coeff
//    ,
//    bool _reverse = true
    ):
    dist(_dist), dim(_dim), vertices(_dim + 1),
    binomial_coeff(_binomial_coeff)
//    ,
//    reverse(_reverse)
    {};
    
    dist_t diam(const int index) {
        dist_t diam = 0;
        get_simplex_vertices(index, dim, dist.size(), binomial_coeff, vertices.begin() );
        
        for (int i = 0; i <= dim; ++i)
            for (int j = i + 1; j <= dim; ++j) {
                auto d = dist(vertices[i], vertices[j]);
                diam = std::max(diam, dist(vertices[i], vertices[j]));
            }
        return diam;
    }

    bool operator()(const int a, const int b)
    {
        assert(a < binomial_coeff(dist.size(), dim + 1));
        assert(b < binomial_coeff(dist.size(), dim + 1));
        
        dist_t a_diam = 0, b_diam = 0;

    
        a_diam = diam(a);
        
        get_simplex_vertices(b, dim, dist.size(), binomial_coeff, vertices.begin() );
        for (int i = 0; i <= dim; ++i)
            for (int j = i + 1; j <= dim; ++j) {
                b_diam = std::max(b_diam, dist(vertices[i], vertices[j]));
                if (a_diam > b_diam)
//                   (((a_diam < b_diam) && !reverse) ||
//                    ((a_diam > b_diam) && reverse))
                    return true;
            }
       
        return (a_diam == b_diam) && (a > b);
//        return (a_diam == b_diam) && (((a < b) && !reverse) || ((a > b) && reverse));
    }
    
};


template<typename OutputIterator>
void get_simplex_coboundary( int idx, int dim, int n, const binomial_coeff_table& binomial_coeff, OutputIterator coboundary )
{

    --n;
    
    int modified_idx = idx;
    
    for( int k = dim + 1; k >= 0 && n >= 0; --k ) {
        while( binomial_coeff( n , k ) > idx ) {
//            std::cout << "binomial_coeff(" << n << ", " << k << ") = " << binomial_coeff( n , k ) << " > " << idx << std::endl;
            *coboundary++ = modified_idx + binomial_coeff( n , k + 1 );
            if (n==0) break;
            --n;
        }
        
        idx -= binomial_coeff( n , k );
            
        modified_idx -= binomial_coeff( n , k );
        modified_idx += binomial_coeff( n , k + 1 );
        
        --n;
    }
 
    return;
}



class distance_matrix  {
public:
    typedef double value_type;
    std::vector<std::vector<double> > distances;
    double operator()(const int a, const int b) const {
        return distances[a][b];
    }
    
    size_t size() const {
        return distances.size();
    }

};


template <typename ValueType>
class compressed_sparse_matrix  {
public:
    std::vector<size_t> bounds;
    std::vector<ValueType> entries;
    
    
    size_t size() const {
        return bounds.size();
    }
    
    typename std::vector<ValueType>::const_iterator cbegin(size_t index) {
        assert(index < size());
        return index == 0 ? entries.cbegin() : entries.cbegin() + bounds[index - 1];
    }

    typename std::vector<ValueType>::const_iterator cend(size_t index) {
        assert(index < size());
        return entries.cbegin() + bounds[index];
    }
    
    template <typename Iterator>
    void append(Iterator begin, Iterator end) {
        for (Iterator it = begin; it != end; ++it) {
            entries.push_back(*it);
        }
        bounds.push_back(entries.size());
    }

    
    template <typename Collection>
    void append(const Collection collection) {
        append(collection.cbegin(), collection.cend());
    }

};

template <typename Heap>
int get_pivot(Heap& column)
{
    if( column.empty() )
        return -1;
    else {
        int max_element = column.top();
        column.pop();
        while( !column.empty() && column.top() == max_element ) {
            column.pop();
            if( column.empty() )
                return -1;
            else {
                max_element = column.top();
                column.pop();
            }
        }
        if( max_element != -1 )
            column.push( max_element );

        return max_element;
    }
}

int main() {

    distance_matrix dist;
//    dist.distances = {
//        {0,1,2,3},
//        {1,0,1,2},
//        {2,1,0,1},
//        {3,2,1,0}
//    };
//    dist.distances = {
//        {0,1,1,1,1},
//        {1,0,1,1,1},
//        {1,1,0,1,1},
//        {1,1,1,0,1},
//        {1,1,1,1,0}
//    };
    dist.distances = {
        {0,1,3,6,4},
        {1,0,8,2,9},
        {3,8,0,10,7},
        {6,2,10,0,5},
        {4,9,7,5,0}
    };
    
    int n = dist.size();
    
    int dim_max = 2;
    binomial_coeff_table binomial_coeff(n, dim_max + 2);
    
    
    std::cout << dist (0,1) << std::endl;
 
    rips_filtration_comparator<distance_matrix> comp1(dist, 1, binomial_coeff);
    rips_filtration_comparator<distance_matrix> comp2(dist, 2, binomial_coeff);
    
    std::cout << (comp1(0,1) ? "0<1" : "0>=1") << std::endl;
    std::cout << (comp1(1,0) ? "1<0" : "1>=0") << std::endl;
    
    std::cout << (comp1(0,2) ? "0<2" : "0>=2") << std::endl;
    std::cout << (comp1(1,2) ? "1<2" : "1>=2") << std::endl;
    
    std::vector<int> edges = {0,1,2,3,4,5};
    
    std::sort(edges.begin(), edges.end(), comp1);
    
    std::cout << "sorted edges: " << edges << std::endl;
   
   
    std::vector<int> triangles = {0,1,2,3};
    
    std::sort(triangles.begin(), triangles.end(), comp2);
    
    std::cout << "sorted triangles: " << triangles << std::endl;
    
    
    int dim = 1;
    int simplex_index = 2;
    
    double threshold = 7;
    
    std::vector<int> vertices;
    
    get_simplex_vertices( simplex_index, dim, n, binomial_coeff, std::back_inserter(vertices) );

    
    std::cout << "coboundary of simplex " << vertices << ":" << std::endl;
    
    std::vector<int> coboundary;
    get_simplex_coboundary( simplex_index, dim, n, binomial_coeff, std::back_inserter(coboundary) );
    
    
    for (int coboundary_simplex_index: coboundary) {
        std::vector<int> vertices;
    
        get_simplex_vertices( coboundary_simplex_index, dim + 1, n, binomial_coeff, std::back_inserter(vertices) );
        std::cout << " " << coboundary_simplex_index << " " << vertices << " (" << comp1.diam(coboundary_simplex_index) << ")" << std::endl;

    }
    
    
    compressed_sparse_matrix<int> csm;
    
    csm.append(std::vector<int>({1,2,3}));
    
    csm.append(std::vector<int>({5,6,7,8}));

    csm.append(std::vector<int>({10,11}));

    csm.append(std::vector<int>());

    csm.append(std::vector<int>({2}));
    
    std::cout << "compressed sparse matrix: " << std::endl;

    for (int i = 0; i < csm.size(); ++i) {
        std::cout << " " << std::vector<int>(csm.cbegin(i), csm.cend(i)) << std::endl;
    }
    
    std::cout << "bounds: " << csm.bounds << std::endl;

    std::cout << "entries: " << csm.entries << std::endl;
    
    
    std::priority_queue<int, std::vector<int>, rips_filtration_comparator<distance_matrix> > queue(comp1);
    
    for (int e: coboundary) queue.push(e);
    
    std::cout << "pivot of coboundary: " << queue.top() << std::endl;
    
    std::cout << (comp1(3,6) ? "3<6" : "3>=6") << std::endl;
    std::cout << (comp1(0,6) ? "0<6" : "0>=6") << std::endl;
    
    
    
    std::vector<int> columns_to_reduce = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    std::sort(columns_to_reduce.begin(), columns_to_reduce.end(), comp1);
    
    std::cout << "sorted columns to reduce: " << columns_to_reduce << std::endl;
    
    for (int index: columns_to_reduce) {
        std::vector<int> vertices;

        get_simplex_vertices( index, dim, n, binomial_coeff, std::back_inserter(vertices) );
        std::cout << " " << index << " " << vertices << " (" << comp1.diam(index) << ")" << std::endl;
    }



    std::unordered_map<int, int> pivot_column_index;
    
    for (auto persistence_pair: pivot_column_index) {
        
    }
    
    int num_simplices = binomial_coeff(n, dim + 1);
    
    columns_to_reduce.clear();
    
    for (int index = 0; index < num_simplices; ++index) {
        if (comp1.diam(index) <= threshold && pivot_column_index.find(index) == pivot_column_index.end()) {
            columns_to_reduce.push_back(index);
        }
    }
    std::sort(columns_to_reduce.begin(), columns_to_reduce.end(), comp1);

    std::cout << "sorted columns to reduce: " << columns_to_reduce << std::endl;
    
    for (int index: columns_to_reduce) {
        std::vector<int> vertices;

        get_simplex_vertices( index, dim, n, binomial_coeff, std::back_inserter(vertices) );
        std::cout << " " << index << " " << vertices << " (" << comp1.diam(index) << ")" << std::endl;
    }


    //compressed_sparse_matrix<int> reduction_matrix;

    std::priority_queue<int, std::vector<int>, rips_filtration_comparator<distance_matrix> > working_coboundary(comp2);
    
    //std::vector<int> reduction_column;
    
    for (int index: columns_to_reduce) {
        //reduction_column.clear();
    
        int pivot, column = index;
        
        do {
        
            coboundary.clear();
            get_simplex_coboundary( column, dim, n, binomial_coeff, std::back_inserter(coboundary) );
            for (int e: coboundary) if (comp2.diam(e) <= threshold) working_coboundary.push(e);
            
            pivot = get_pivot(working_coboundary);
            

            //add boundary column at index birth to working_coboundary
            //since the boundary column is not stored explicitly,
            //add the boundary of the column at index birth in the reduction matrix instead
            
            //space-efficient variant: add just the boundary column at index birth instead
            //this avoids having to store the reduction matrix
            
            if (pivot != -1) {
                auto pair = pivot_column_index.find(pivot);
                
                if (pair == pivot_column_index.end()) {
                    pivot_column_index.insert(std::make_pair(pivot, index));
                    break;
                }
                
                column = pair->second;
            }
            
            /*
            coboundary.clear();
            get_simplex_coboundary( birth, dim, n, binomial_coeff, std::back_inserter(coboundary) );
            for (int e: coboundary) if (comp1.diam(e) <= threshold) working_coboundary.push(e);

            //space-efficient variant: drop this part (and the reduction_matrix)
            
            for (int col = reduction_matrix.cbegin()) {
                coboundary.clear();
                get_simplex_coboundary( col, dim, n, binomial_coeff, std::back_inserter(coboundary) );
                for (int e: coboundary) if (comp1.diam(e) <= threshold) working_coboundary.push(e);
            }
             */
        } while ( pivot != -1 );

    }
    
    std::cout << "pairs:" << std::endl << pivot_column_index << std::endl;
    
    
}