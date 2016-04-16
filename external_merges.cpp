#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <queue>

using std::size_t;

void MergeStreams(std::istream &input1, std::istream &input2, std::ostream &output) {
	long long x1, x2;
	input1 >> x1;
	input2 >> x2;

	while (true) {
		if (x1 <= x2) {
			output << x1 << std::endl;
			if (!(input1 >> x1)) {
				output << x2 << std::endl;
				while (input2 >> x2) {
					output << x2 << std::endl;
				}
				return;
			}
		} else {
			output << x2 << std::endl;
			if (!(input2 >> x2)) {
				output << x1 << std::endl;
				while (input2 >> x2) {
					output << x2 << std::endl;
				}
				return;
			}
		}
	}
}

template <typename T>
struct Mergee {
	T value;
	size_t key;
};

template <typename T>
class mergeeComparison {
  bool reverse;
public:
  mergeeComparison(const bool& revparam=false) : reverse(revparam) {}

  bool operator() (Mergee<T> &m1, Mergee<T> &m2) const {
    if (reverse) return (m1.value > m2.value);
    else return m1.value < m2.value;
  }
};

template <typename T>
void MergeSort(std::fstream &file, size_t size) {
	if (size == 0) {
		return;
	}

	size_t bufferSize = 3;//1 << 10;
	bufferSize = std::min(bufferSize, 2 * size);
	std::vector<T> buffer(bufferSize);

	////////////////// per-block sort ////////////////////
	for (size_t chunkStart = 0; chunkStart < size; chunkStart += bufferSize) {
		size_t chunkEnd = std::min(chunkStart + bufferSize, size);
		file.seekg(chunkStart*sizeof(T));
		file.read((char*)&buffer[0], (chunkEnd - chunkStart)*sizeof(T));
		std::sort(buffer.begin(), buffer.begin() + chunkEnd - chunkStart);
		file.seekp(chunkStart*sizeof(T));
		file.write((char*)&buffer[0], (chunkEnd - chunkStart)*sizeof(T));
	}

	///////////////// k-way merge /////////////////////
	size_t k = (size - 1)/bufferSize + 1; // TODO: limit k?
	size_t bufferChunkSize = bufferSize / (k + 1);
	size_t outBufOffset = bufferChunkSize * k;

	std::vector<size_t> bufferChunkOffsets(k), bufferChunkEnds(k);
	std::vector<size_t> fileChunkOffsets(k), fileChunkEnds(k);
	std::priority_queue<Mergee<T>, std::vector<Mergee<T>>, mergeeComparison<T> > mergeMins(mergeeComparison<T>(true));

	for (size_t i = 0; i < k; ++i) {
		bufferChunkOffsets[i] = i * bufferChunkSize;
		bufferChunkEnds[i] = (i + 1) * bufferChunkSize;
		fileChunkOffsets[i] = i * bufferSize;
		fileChunkEnds[i] = (i + 1) * bufferSize;
	}
	fileChunkEnds[k-1] = size;

	for (size_t i = 0; i < k; ++i) {
		file.seekg(fileChunkOffsets[i]*sizeof(T));
		file.read((char*)&buffer[bufferChunkOffsets[i]], bufferChunkSize*sizeof(T));
		mergeMins.push({buffer[bufferChunkOffsets[i]], i});
		bufferChunkOffsets[i]++;
		fileChunkOffsets[i]++;
	}
	size_t fileOutputOffset = 0;

	while (fileOutputOffset < size) {
		// pop min
		Mergee<T> min = mergeMins.top();
		mergeMins.pop();
		// write it in buffer
		buffer[outBufOffset++] = min.value;
		// flush buffer if necessary
		if (outBufOffset == bufferSize) {
			// reset output buffer offset
			outBufOffset = bufferChunkSize * k;
			// flush output
			file.seekp(fileOutputOffset*sizeof(T));
			file.write((char*)&buffer[outBufOffset], (bufferSize - outBufOffset)*sizeof(T));
			fileOutputOffset += (bufferSize - outBufOffset);
		}

		// read file if necessary
		if (bufferChunkOffsets[min.key] == bufferChunkEnds[min.key]) {
			if (fileChunkOffsets[min.key] == fileChunkEnds[min.key]) {
				continue; // file chunk ended
			}
			bufferChunkOffsets[min.key] = min.key * bufferChunkSize; // reload bufferChunkOffset
			size_t toRead = std::min(bufferChunkSize, fileChunkEnds[min.key]-fileChunkOffsets[min.key]);
			file.seekg(fileChunkOffsets[min.key]*sizeof(T));
			file.read((char*)&buffer[bufferChunkOffsets[min.key]], toRead*sizeof(T));
			fileChunkOffsets[min.key] += toRead;
		}
		// push new min
		mergeMins.push({buffer[bufferChunkOffsets[min.key]], min.key});
		bufferChunkOffsets[min.key]++;
	}
	file.seekp(fileOutputOffset*sizeof(T));
	if (outBufOffset - bufferChunkSize * k != fileOutputOffset - size) {
		std::cout << outBufOffset << std::endl;
		std::cout << bufferChunkSize * k << std::endl;
		std::cout << fileOutputOffset - size << std::endl;
		throw 1;
	}
	file.write((char*)&buffer[bufferChunkSize * k], (outBufOffset - bufferChunkSize * k)*sizeof(T));

}

template <typename T>
std::vector<T> ReadVector(std::fstream &file, size_t size) {
	std::vector<T> output(size);
	file.seekg(0, file.beg);
	for (size_t i = 0; i < size; ++i) {
		file.read((char*)&output[i], sizeof(T));
	}
	return output;
}

template <typename T>
void PrintVector(std::vector<T> &vect) {
	for (T &val : vect) {
		std::cout << val << ' ';
	}
}

template <typename T>
void WriteVector(std::vector<T> &vect, std::fstream &file) {
	file.seekp(0);
	for (T &val : vect) {
		file.write((char*)&val, sizeof(T));
	}
}

int main() {
	std::fstream file("data3.bin", std::ios::in | std::ios::out | std::ios::binary);
	std::vector<long> v({1,2,3,6,5});
	WriteVector(v, file);

	MergeSort<long>(file, 5);

	auto ov = ReadVector<long>(file, 5);
	PrintVector(ov);

	return 0;
}
