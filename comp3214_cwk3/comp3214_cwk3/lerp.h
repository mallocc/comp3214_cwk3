#pragma once


#include "glm.h"
#include <minmax.h>





struct Lerper
{
public:
	glm::vec3 start, end;
	float step = 0;
	float t = 0;
	float wait = 0;
	bool finished = 0;

	Lerper(glm::vec3 start, glm::vec3 end, float step)
	{
		this->start = start;
		this->end = end;
		this->step = step;
	}
	Lerper(glm::vec3 start, glm::vec3 end, float step, float wait)
	{
		this->start = start;
		this->end = end;
		this->step = step;
		this->wait = wait;
	}

	void reset()
	{
		finished = t = 0;
	}

	bool isFinished()
	{
		return finished;
	}

	glm::vec3 lerpStepSmooth()
	{
		if (t < 1)
		{
			t += step;
			return start + (end - start) * smoothstep(0, 1, t);
		}
		stepWait();
		return end;
	}

	glm::vec3 lerpStep()
	{
		if (t < 1)
		{
			t += step;
			return start + (end - start) * t;
		}
		stepWait();
		return end;
	}

private:
	float clamp(float x, float lowerlimit, float upperlimit) {
		if (x < lowerlimit)
			x = lowerlimit;
		if (x > upperlimit)
			x = upperlimit;
		return x;
	}
	float smoothstep(float edge0, float edge1, float x) {
		// Scale, bias and saturate x to 0..1 range
		x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
		// Evaluate polynomial
		return x * x * (3 - 2 * x);
	}
	
	void stepWait()
	{
		if (t < 1 + wait)
			t += step;
		else
		{
			t = 1 + wait;
			finished = 1;
		}		
	}
};


struct LerperSequencer
{
	std::vector<Lerper> sequence;
	int currentLerper = 0;

	LerperSequencer() {}

	LerperSequencer(glm::vec3 start, glm::vec3 first, float step, float wait) 
	{
		addLerper(Lerper(start, first, step, wait));
	}

	LerperSequencer(std::vector<Lerper> sequence)
	{
		this->sequence = sequence;
	}

	void addLerper(Lerper lerper)
	{
		sequence.push_back(lerper);
	}
	void addLerper(glm::vec3 next, float step, float wait)
	{
		sequence.push_back(Lerper(sequence.back().end, next, step, wait));
	}
	void addLerper(glm::vec3 next)
	{
		sequence.push_back(Lerper(sequence.back().end, next, sequence.front().step, sequence.front().wait));
	}
	void addLerper(glm::vec3 next, float step)
	{
		sequence.push_back(Lerper(sequence.back().end, next, step, sequence.front().wait));
	}
	void addLerper(float wait, glm::vec3 next)
	{
		sequence.push_back(Lerper(sequence.back().end, next, sequence.front().step, wait));
	}

	void reset()
	{
		currentLerper = 0;
		for (Lerper l : sequence)
			l.reset();
	}

	glm::vec3 lerpStep()
	{
		if (sequence.size() > 0)
		{
			if (sequence[currentLerper].isFinished())
			{
				if (currentLerper == sequence.size() - 1)
					return sequence[currentLerper].end;
				else
					currentLerper++;
			}
			return sequence[currentLerper].lerpStep();
		}
		return glm::vec3();
	}

	glm::vec3 lerpStepSmooth()
	{
		if (sequence.size() > 0)
		{
			if (sequence[currentLerper].isFinished())
			{
				if (currentLerper == sequence.size() - 1)
					return sequence[currentLerper].end;
				else
					currentLerper++;				
			}
			return sequence[currentLerper].lerpStepSmooth();
		}
		return glm::vec3();
	}

};