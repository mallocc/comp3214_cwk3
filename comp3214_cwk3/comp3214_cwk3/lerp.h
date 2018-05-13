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

	Lerper() {}

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

	glm::vec3 lerpStepSmooth(float step)
	{
		if (t < 1)
		{
			t += step;
			return start + (end - start) * smoothstep(0, 1, t);
		}
		stepWait(step);
		return end;
	}

	glm::vec3 lerpStep(float step)
	{
		if (t < 1)
		{
			t += step;
			return start + (end - start) * t;
		}
		stepWait(step);
		return end;
	}

	glm::vec3 lerpStepSmooth()
	{
		return lerpStepSmooth(step);
	}

	glm::vec3 lerpStep()
	{
		return lerpStep(step);
	}

	glm::vec3 getStart()
	{
		return start;
	}

	glm::vec3 getEnd()
	{
		return end;
	}

	float getStep()
	{
		return step;
	}

	float getWait()
	{
		return wait;
	}

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
	
	void stepWait(float step)
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

struct BezierLerper : Lerper
{
	//Lerper line1, line2;

	//BezierLerper(glm::vec3 start, glm::vec3 control, glm::vec3 end, float step, float wait)
	//{
	//	line1 = Lerper(start, control, step, wait);
	//	line2 = Lerper(control, end, step, wait);
	//}

	//glm::vec3 lerpStepSmooth()
	//{
	//	if (!line1.isFinished())
	//	{
	//		glm::vec3 p1 = line1.lerpStepSmooth();
	//		glm::vec3 p2 = line2.lerpStepSmooth();
	//		return p1 + (p2 - p1) * smoothstep(0,1,line1.t);
	//	}
	//	return line2.getEnd();
	//}

	//glm::vec3 lerpSmooth()
	//{
	//	if (!line1.isFinished())
	//	{
	//		glm::vec3 p1 = line1.lerpStep();
	//		glm::vec3 p2 = line2.lerpStep();
	//		return p1 + (p2 - p1) * line1.t;
	//	}
	//	line1.stepWait();
	//	line2.stepWait();
	//	return line2.getEnd();
	//}

	//bool isFinished()
	//{
	//	return line1.isFinished() && line2.isFinished();
	//}

	//glm::vec3 getStart()
	//{
	//	return line1.getStart();
	//}

	//glm::vec3 getEnd()
	//{
	//	return line2.getEnd();
	//}

	//float getStep()
	//{
	//	return line1.getStep();
	//}

	//float getWait()
	//{
	//	return line1.getWait();
	//}

	//void reset()
	//{
	//	line1.reset();
	//	line2.reset();
	//}

};


struct LerperSequencer
{
public:
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
		sequence.push_back(Lerper(sequence.back().getEnd(), next, step, wait));
	}
	void addLerper(glm::vec3 next)
	{
		sequence.push_back(Lerper(sequence.back().getEnd(), next, sequence.front().getStep(), sequence.front().getWait()));
	}
	void addLerper(glm::vec3 next, float step)
	{
		sequence.push_back(Lerper(sequence.back().getEnd(), next, step, sequence.front().getWait()));
	}
	void addLerper(float wait, glm::vec3 next)
	{
		sequence.push_back(Lerper(sequence.back().getEnd(), next, sequence.front().getStep(), wait));
	}

	//void addLerper(glm::vec3 control, glm::vec3 next)
	//{
	//	sequence.push_back(BezierLerper(sequence.back().getEnd(), control, next, sequence.front().getStep(), sequence.front().getWait()));
	//}

	void reset()
	{
		currentLerper = 0;
		for (Lerper l : sequence)
			l.reset();
	}

	bool isFinished()
	{
		return sequence.size() > 0 && sequence[sequence.size() - 1].isFinished();
	}

	glm::vec3 lerpStep(float step)
	{
		if (sequence.size() > 0)
		{
			if (sequence[currentLerper].isFinished())
			{
				if (currentLerper == sequence.size() - 1)
					return sequence[currentLerper].getEnd();
				else
					currentLerper++;
			}
			return sequence[currentLerper].lerpStep(step);
		}
		return glm::vec3();
	}

	glm::vec3 lerpStepSmooth(float step)
	{
		if (sequence.size() > 0)
		{
			if (sequence[currentLerper].isFinished())
			{
				if (currentLerper == sequence.size() - 1)
					return sequence[currentLerper].getEnd();
				else
					currentLerper++;				
			}
			return sequence[currentLerper].lerpStepSmooth(step);
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
					return sequence[currentLerper].getEnd();
				else
					currentLerper++;
			}
			return sequence[currentLerper].lerpStepSmooth();
		}
		return glm::vec3();
	}

	glm::vec3 lerpStep()
	{
		if (sequence.size() > 0)
		{
			if (sequence[currentLerper].isFinished())
			{
				if (currentLerper == sequence.size() - 1)
					return sequence[currentLerper].getEnd();
				else
					currentLerper++;
			}
			return sequence[currentLerper].lerpStep();
		}
		return glm::vec3();
	}

	void next()
	{
		if (sequence.size() > 0)
			if (currentLerper < sequence.size() - 1)
				currentLerper++;
	}
};


struct CameraSequencer : LerperSequencer
{
public:
	std::vector<Lerper> directions;
	std::vector<Lerper> ups;
	std::vector<Lerper> others;

	glm::vec3 currentPosition, currentDirection, currentUp, currentOther;

	CameraSequencer() : LerperSequencer() {}

	CameraSequencer(
		glm::vec3 startPos, glm::vec3 endPos,	glm::vec3 startDir, glm::vec3 endDir, glm::vec3 startUp, glm::vec3 endUp, glm::vec3 startOther, glm::vec3 endOther, float step, float wait)
	{
		addLerper(Lerper(startPos, endPos, step, wait), Lerper(startDir, endDir, step, wait), Lerper(startUp, endUp, step, wait), Lerper(startOther, endOther, step, wait));
	}

	void addLerper(Lerper position, Lerper direction, Lerper up, Lerper other)
	{
		sequence.push_back(position);
		directions.push_back(direction);
		ups.push_back(up);
		others.push_back(other);
	}
	void addLerper(glm::vec3 nextPos, glm::vec3 nextDir, glm::vec3 nextUp, float step, float wait)
	{
		addLerper(Lerper(sequence.back().getEnd(), nextPos, step, wait), Lerper(directions.back().getEnd(), nextDir, step, wait), Lerper(ups.back().getEnd(), nextUp, step, wait), Lerper(others.back().getEnd(), ups.back().getEnd(), step, wait));
	}
	void addLerper(glm::vec3 nextPos, glm::vec3 nextDir, glm::vec3 nextUp, glm::vec3 nextOther, float step, float wait)
	{
		addLerper(Lerper(sequence.back().getEnd(), nextPos, step, wait), Lerper(directions.back().getEnd(), nextDir, step, wait), Lerper(ups.back().getEnd(), nextUp, step, wait), Lerper(others.back().getEnd(), nextOther, step, wait));
	}

	//void addLerper(glm::vec3 nextPos, glm::vec3 control, glm::vec3 nextDir, glm::vec3 nextUp, float step, float wait)
	//{
	//	addLerper(BezierLerper(sequence.back().getEnd(), control, nextPos, step, wait), Lerper(directions.back().getEnd(), nextDir, step, wait), Lerper(ups.back().getEnd(), nextUp, step, wait), Lerper(ups.back().getEnd(), ups.back().getEnd(), step, wait));
	//}

	void reset()
	{
		currentLerper = 0;
		for (Lerper l : sequence)
			l.reset();
		for (Lerper l : directions)
			l.reset();
		for (Lerper l : ups)
			l.reset();
		for (Lerper l : others)
			l.reset();
	}

	void lerpStep(float step)
	{
		if (sequence.size() > 0)
		{
			if (sequence[currentLerper].isFinished())
			{
				if (currentLerper == sequence.size() - 1)
				{
					currentPosition = sequence[currentLerper].getEnd();
					currentDirection = directions[currentLerper].getEnd();
					currentUp = ups[currentLerper].getEnd();
					currentOther = others[currentLerper].getEnd();
				}
				else
					currentLerper++;
			}
			else {
				currentPosition = sequence[currentLerper].lerpStep(step);
				currentDirection = directions[currentLerper].lerpStep(step);
				currentUp = ups[currentLerper].lerpStep(step);
				currentOther = others[currentLerper].lerpStep(step);
			}
		}
		else
			currentPosition = currentDirection = currentUp = currentOther = glm::vec3();
	}

	void lerpStepSmooth(float step)
	{
		if (sequence.size() > 0)
		{
			if (sequence[currentLerper].isFinished())
			{
				if (currentLerper == sequence.size() - 1)
				{
					currentPosition = sequence[currentLerper].getEnd();
					currentDirection = directions[currentLerper].getEnd();
					currentUp = ups[currentLerper].getEnd();
					currentOther = others[currentLerper].getEnd();
				}
				else
					currentLerper++;
			}
			else {
				currentPosition = sequence[currentLerper].lerpStepSmooth(step);
				currentDirection = directions[currentLerper].lerpStepSmooth(step);
				currentUp = ups[currentLerper].lerpStepSmooth(step);
				currentOther = others[currentLerper].lerpStepSmooth(step);
			}
		}
		else
			currentPosition = currentDirection = currentUp = currentOther = glm::vec3();
	}

	void lerpStep()
	{
		if (sequence.size() > 0)
		{
			if (sequence[currentLerper].isFinished())
			{
				if (currentLerper == sequence.size() - 1)
				{
					currentPosition = sequence[currentLerper].getEnd();
					currentDirection = directions[currentLerper].getEnd();
					currentUp = ups[currentLerper].getEnd();
					currentOther = others[currentLerper].getEnd();
				}
				else
					currentLerper++;
			}
			else {
				currentPosition = sequence[currentLerper].lerpStep();
				currentDirection = directions[currentLerper].lerpStep();
				currentUp = ups[currentLerper].lerpStep();
				currentOther = others[currentLerper].lerpStep();
			}
		}
		else
			currentPosition = currentDirection = currentUp = currentOther = glm::vec3();
	}

	void lerpStepSmooth()
	{
		if (sequence.size() > 0)
		{
			if (sequence[currentLerper].isFinished())
			{
				if (currentLerper == sequence.size() - 1)
				{
					currentPosition = sequence[currentLerper].getEnd();
					currentDirection = directions[currentLerper].getEnd();
					currentUp = ups[currentLerper].getEnd();
					currentOther = others[currentLerper].getEnd();
				}
				else
					currentLerper++;
			}
			else {
				currentPosition = sequence[currentLerper].lerpStepSmooth();
				currentDirection = directions[currentLerper].lerpStepSmooth();
				currentUp = ups[currentLerper].lerpStepSmooth();
				currentOther = others[currentLerper].lerpStepSmooth();
			}
		}
		else
			currentPosition = currentDirection = currentUp = currentOther = glm::vec3();
	}

private:

};
