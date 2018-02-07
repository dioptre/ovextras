
#pragma once

class Range {
public:
	Range(uint64_t start, uint64_t end) : m_startTime(start), m_endTime(end) { };

	uint64_t m_startTime;
	uint64_t m_endTime;

private: 
	Range(void);

};

