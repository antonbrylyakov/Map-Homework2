#include <iostream>
#ifdef _WIN32
#include "windows.h"
#endif

#include <vector>
#include <thread>
#include <stdexcept>
#include <mutex>
#include <random>
#include <iomanip>
#include "Measurements.hpp"


void vectorSum(std::once_flag& ofl, std::vector<int>& v1, std::vector<int>& v2, std::vector<int>& result, int threadCount = 1, bool useMainThread = true)
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

	auto claculatePart = [&v1, &v2, &result, &ofl](size_t start, size_t end)
	{
		std::call_once(ofl, []() { std::cout << "Количество аппаратных ядер - " << std::thread::hardware_concurrency() << std::endl << std::endl; });

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

		if (i < threadCount - 1 && !useMainThread)
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

// Заполняет вектор случайными значениями
void populateVector(std::vector<int>& v)
{
	std::mt19937 gen(std::chrono::steady_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<int> dis(-100, 100);
	auto rand_num([&dis, &gen]() mutable { return dis(gen); });
	std::generate(v.begin(), v.end(), rand_num);
}


int main()
{
	setlocale(LC_ALL, "Russian");
#ifdef _WIN32
	SetConsoleCP(1251);
#endif

	std::once_flag ofl;
	size_t repeatCount = 10;
	size_t threadCounts[] = { 1, 2, 4, 8, 16 };
	size_t sizes[] = { 1000, 1'0000, 100'000, 1'000'000 };
	auto sizesCnt = sizeof(sizes) / sizeof(sizes[0]);

	// создаем структуру данных для сохранения результатов измерений
	std::vector<MeasurementSet<double>> measurements;
	for (auto& sz : sizes)
	{
		measurements.push_back(MeasurementSet<double>());
	}
		
	for (size_t szn = 0; szn < sizesCnt; ++szn)
	{
		auto sz = sizes[szn];
		for (auto tc : threadCounts)
		{
			// Вычисления repeatCount раз

			for (size_t i = 0; i < repeatCount; ++i)
			{
				std::vector<int> v1(sz);
				populateVector(v1);
				std::vector<int> v2(sz);
				populateVector(v2);

				std::vector<int> result(sz);
				auto start = std::chrono::high_resolution_clock::now();
				vectorSum(ofl, v1, v2, result, tc, false);
				auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> time = end - start;

				measurements[szn].add(MeasurementItem<double>{ tc, time.count() });
			}
		}
	}


	// Выводим результаты	
	std::cout << "\t\t";
	for (auto sz : sizes)
	{
		std::cout << sz << "\t\t";
	}

	std::cout << std::endl;

	for (auto tc : threadCounts)
	{
		std::cout << tc << " поток(а,ов)" << "\t";
		for (size_t szn = 0; szn < sizesCnt; ++szn)
		{
			auto sz = sizes[szn];
			auto avgTime = measurements[szn].get(tc);
			if (avgTime.has_value())
			{
				std::cout << std::fixed << std::setprecision(2) << avgTime.value().value << "ms\t\t";
			}
			else
			{
				std::cout << "--\t\t";
			}
		}

		std::cout << std::endl;
	}

	std::cout << "Опт. потоков" << "\t";

	for (size_t szn = 0; szn < sizesCnt; ++szn)
	{
		auto optimum = measurements[szn].getOptimum();
		if (optimum.has_value())
		{
			std::cout << optimum.value().threadCount << "\t\t";
		}
		else
		{
			std::cout << "--\t\t";
		}
	}
}