#include <iostream>
#ifdef _WIN32
#include "windows.h"
#endif

#include <vector>
#include <thread>
#include <stdexcept>
#include <mutex>
#include <random>



void vectorSum(std::vector<int>& v1, std::vector<int>& v2, std::vector<int>& result, int threadCount = 1)
{
	if (threadCount <= 0)
	{
		throw std::invalid_argument("Количество потоков должно быть не менее одного");
	}

	if (v1.size() != v2.size())
	{
		throw std::invalid_argument("Длины входных векторов должны совпадать");
	}

	if (result.size() < v1.size())
	{
		throw std::invalid_argument("Длина выходного вектора должна быть не менее длины входных векторов");
	}

	auto claculatePart = [&v1, &v2, &result](size_t start, size_t end)
	{
		for (auto i = start; i < end; ++i)
		{
			result[i] = v1[i] + v2[i];
		}

	};

	std::vector<std::thread> tv;

	auto wholeChunkSz = v1.size() / threadCount;
	auto remSz = v1.size() % threadCount;
	auto startIndex = 0;

	for (auto i = 0; i < threadCount; ++i)
	{
		// примерно распределяем диапазоны расчетов по потокам
		auto endIndex = startIndex + wholeChunkSz;
		// Добавляем по одному элементу из остатка, который не распределился равномерно
		if (remSz > 0)
		{
			--remSz;
			++endIndex;
		}

		if (i < threadCount - 1)
		{
			tv.emplace_back(claculatePart, startIndex, endIndex);
			startIndex = endIndex;
		}
		else
		{
			// Используем также основной поток, чтобы не тратить его время только на ожидание
			claculatePart(startIndex, endIndex);
		}
	}

	for (auto& t : tv)
	{
		t.join();
	}
}

int main()
{
	setlocale(LC_ALL, "Russian");
#ifdef _WIN32
	SetConsoleCP(1251);
#endif

	int threadCounts[] = { 1, 2, 4, 8, 16 };
	int sizes[] = { 1000, 10000, 100000, 1000000 };

	std::cout << "Количество аппаратных ядер - " << std::thread::hardware_concurrency() << std::endl << std::endl;

	std::cout << "\t\t";
	for (auto sz : sizes)
	{
		std::cout << sz << "\t\t";
	}

	std::cout << std::endl;

	for (auto tc : threadCounts)
	{
		std::cout << tc << " поток(а,ов)" << "\t";
		for (auto sz : sizes)
		{
			// Заполняем вектора случайными данными
			std::vector<int> v1(sz);
			std::vector<int> v2(sz);
			std::vector<int> result(sz);
			std::mt19937 gen(std::chrono::steady_clock::now().time_since_epoch().count());
			std::uniform_int_distribution<int> dis(0, sz);
			auto rand_num([&dis, &gen]() mutable { return dis(gen); });
			std::generate(v1.begin(), v1.end(), rand_num);
			gen = std::mt19937(std::chrono::steady_clock::now().time_since_epoch().count());
			std::generate(v2.begin(), v2.end(), rand_num);

			// Начинаем вычисления

			auto start = std::chrono::high_resolution_clock::now();
			vectorSum(v1, v2, result, tc);
			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> time = end - start;
			std::cout << time.count() << "s\t";
		}

		std::cout << std::endl;
	}
}