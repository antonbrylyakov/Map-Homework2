#include <optional>
#include <map>


template <typename T>
class Average
{
public:
	void add(T value)
	{
		m_sum += value;
		m_count++;
	}

	std::optional<T> getResult()
	{
		return m_count > 0 ? std::optional<T>(m_sum / m_count) : std::optional<T>();
	}
private:
	size_t m_count = 0;
	T m_sum = 0;
};

template <typename T>
struct MeasurementItem
{
	// Количество потоков
	size_t threadCount;
	// Время или другая характеристика
	T value;
};

template <typename T>
class MeasurementSet
{
public:
	MeasurementSet() : m_map()
	{

	}

	void add(MeasurementItem<T> mi)
	{
		if (!m_map.contains(mi.threadCount))
		{
			m_map[mi.threadCount] = MeasurementItem<Average<T>>();
		}

		auto& avg = m_map.at(mi.threadCount);
		avg.threadCount = mi.threadCount;
		avg.value.add(mi.value);

		if (!m_optimum.has_value() || m_optimum.value().value > mi.value)
		{
			m_optimum = std::optional<MeasurementItem<T>>(mi);
		}
	}

	std::optional<MeasurementItem<T>> get(size_t threadCount)
	{
		if (m_map.contains(threadCount))
		{
			auto avgMi = m_map[threadCount].value.getResult();
			if (avgMi.has_value())
			{

				MeasurementItem<T> mi;
				mi.threadCount = threadCount;
				mi.value = avgMi.value();
				return std::optional<MeasurementItem<T>>(mi);
			}
		}

		return std::optional<MeasurementItem<T>>();
	}

	std::optional<MeasurementItem<T>> getOptimum()
	{
		return m_optimum;
	}
private:
	std::map<size_t, MeasurementItem<Average<T>>> m_map;
	std::optional<MeasurementItem<T>> m_optimum;
};