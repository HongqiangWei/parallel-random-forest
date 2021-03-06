#include "RandomForest.h"
#include <cmath>
#include <cstdlib>

RandomForest::RandomForest(size_t numOfTrees, size_t maxValues, size_t numLabels,
             double sampleCoeff) {
    this->numOfTrees = numOfTrees;
    this->maxValues = maxValues;
    this->numLabels = numLabels;
    this->sampleCoeff = sampleCoeff;
    this->forest.resize(numOfTrees);
};


void RandomForest::fit(Values &X, Labels &y, const Indices &ids) {
    bootstrap = sample(ids);
#ifdef DEBUG_FOREST
    printf("Bootstraping done. Sample size = %d\n", bootstrap.size());
    printf("Start training...\nTrees completed:\n");
#endif
    // draw a random sample from X and y

//TODO: parallel
#pragma omp parallel for
    for (int i = 0; i < numOfTrees; ++i) {

    const IndicesSet features = chooseFeaturess(FEATURE_NUM, maxValues);
#ifdef DEBUG_TREE
    printf("Chosen features: ");
    for (auto &f : features) {
        printf("%d ", f);
    }
    printf("\n");
#endif
        DecisionTree tree;
        // train a tree with the sample
        tree.fit(X, y, bootstrap, features);
        // put it into the forest
        forest[i] = tree;
#ifdef DEBUG_TREE
        tree.print();
#endif
#ifdef DEBUG_FOREST
        printf("================   %d   ==============\n", i);
#endif
    }

#ifdef DEBUG_FOREST
    printf("\nTraining done.\n");
#endif
}

Indices RandomForest::sample(const Indices &ids) {
    size_t data_size = ids.size();
    size_t sample_size = (int)(sampleCoeff * data_size);
    Indices idx;

    for (int i = 0; i < sample_size; ++i) {
        size_t next = rand() % data_size;  // with replacement
        idx.push_back(next);
    }
    return idx;
}


IndicesSet RandomForest::chooseFeaturess(size_t numValues, size_t maxValues) {
    // randomly choose maxValues numbers from [0, numValues - 1]
    Indices idx(numValues);
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

    for (size_t i = 0; i < numValues; ++i) idx[i] = i;
    std::shuffle(idx.begin(), idx.end(), std::default_random_engine(seed));

    return IndicesSet(idx.begin(), idx.begin() + maxValues);
}


MutLabels RandomForest::predict(Values &X) {
    int total = X.size();
    MutLabels y(total);
#ifdef DEBUG_FOREST
    printf("\nStart prediction, data size %d...\n", X.size());
#endif

// parallel
#pragma omp parallel for
    for (int i = 0; i < total; ++i) {
        y[i] = predict(X[i]);
    }
#ifdef DEBUG_FOREST
    printf("Prediction completed.\n");
#endif
    return y;
}

int RandomForest::predict(Row &x) {
    // get the prediction from all tress
    MutLabels results(numOfTrees);

    for (int i = 0; i < numOfTrees; ++i) {
        results[i] = forest[i].predict(x);
    }

    Counter counter(results);

    // average
    int result = counter.getMostFrequent();
    return result;
}

bool csv2data(const char* filename, MutValues &X, MutLabels &y,
              Indices &ids, int idIdx, int labelIdx) {
    std::ifstream file(filename);

    if (!file) {
    #ifdef JDEBUG
        printf("Failed to open the file.\n");
    #endif
        return false;
    }

    #ifdef JDEBUG
    printf("Skipping the first line...\n");
    #endif
    string line, value;
    std::getline(file, line); // first line

    #ifdef JDEBUG
    printf("Start loading the data...\n");
    #endif
    // for each line
    size_t row = 0;
    while (std::getline(file, line)) {
        MutRow temp;
        std::istringstream ss(line);

        // split by commas
        // Note: here I use strtol and strtod instead of std::stoi
        //       and std::stod because mingw32 has a bug and doesn't
        //       have these. g++ under linux works find though.
        size_t col = 0;
        size_t row_size = labelIdx == -1 ? FEATURE_NUM + 1 : FEATURE_NUM + 2;
        for (size_t i = 0; i < row_size; ++i) {
            std::getline(ss, value, ',');
            if (i == idIdx) {  // id
                if (labelIdx == -1) {
                  ids.push_back(strtol(value.c_str(), nullptr, 10));  // real id
                } else {
                    ids.push_back(row);  // idx
                }
            } else if (i == labelIdx) {  // label
                y.push_back(strtol(value.c_str(), nullptr, 10));
            } else { // value
                temp[col] = strtod(value.c_str(), nullptr);
                col++;
            }
        }
        X.push_back(temp);
        row++;
    }
#ifdef JDEBUG
    printf("Data loading completed.\nThe first line is:\n");
    printRow(X[0]);
    printf("The last line is::\n");
    printRow(X[X.size() - 1]);
#endif
    return true;
}

void printRow(Row row) {
    printf("[\t");
    size_t row_size = row.size();
    if (row_size < 11) {
        for (auto &value : row) {
            printf("%f\t", value);
        }
    } else {
        for (size_t i = 0; i < 5; ++i) {
            printf("%f\t", row[i]);
        }
        printf("\t...\t");
        for (size_t i = row_size - 5; i < row_size; ++i) {
            printf("%f\t", row[i]);
        }
    }
    printf("]\n");
}