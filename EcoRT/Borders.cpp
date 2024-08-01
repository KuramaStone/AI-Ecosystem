#include "Borders.h"

Triangle::Triangle(b2Vec3 a, b2Vec3 b, b2Vec3 c) :
	a(a), b(b), c(c) {

}

RectangleBorders::RectangleBorders(float x, float y, float w, float h) :
	x(x), y(y), width(w), height(h) {

}

CaveLine::CaveLine(float x1, float y1, float x2, float y2) :
	x1(x1), y1(y1), x2(x2), y2(y2) {
}

b2Vec2 CaveLine::getHashingPosition() {
	// return center
	return b2Vec2((x1 + x2) / 2, (y1 + y2) / 2);
}

float CaveLine::getHashSize() {
	// return length of lines.
	return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

int LineOptimizer::areEndpointsEqual(CaveLine* line1, CaveLine* line2) {
	const float epsilon = 0.001;

	if (std::abs(line1->x2 - line2->x1) < epsilon && std::abs(line1->y2 - line2->y1) < epsilon)
		return 1;
	if (std::abs(line1->x1 - line2->x2) < epsilon && std::abs(line1->y1 - line2->y2) < epsilon) // start of l1 to end of l2
		return 2;

	return 0;
}

// merges line1  into line2
void LineOptimizer::mergeLines(CaveLine* line1, CaveLine* line2, bool flip) {

	CaveLine result = CaveLine(0, 0, 0, 0);

	if (flip) {
		result.x1 = line2->x1;
		result.y1 = line2->y1;
		result.x2 = line1->x2;
		result.y2 = line1->y2; 
	}
	else {
		result.x1 = line1->x1;
		result.y1 = line1->y1;
		result.x2 = line2->x2;
		result.y2 = line2->y2;
	}

	line2->x1 = result.x1;
	line2->y1 = result.y1;
	line2->x2 = result.x2;
	line2->y2 = result.y2;
}

std::vector<CaveLine*> LineOptimizer::mergeRedundantLines(std::vector<CaveLine*>& lines, float angleThreshold) {
	std::vector<CaveLine*> mergedLines;

	for (CaveLine* currentLine : lines) {
		bool merged = false;
		float angle1 = atan2f(currentLine->y2 - currentLine->y1, currentLine->x2 - currentLine->x1);
		float length1 = powf(currentLine->x2 - currentLine->x1, 2) + powf(currentLine->y2 - currentLine->y1, 2);

		for (CaveLine* mergedLine : mergedLines) {
			float length2 = powf(currentLine->x2 - currentLine->x1, 2) + powf(currentLine->y2 - currentLine->y1, 2);
			float angle2 = atan2f(mergedLine->y2 - mergedLine->y1, mergedLine->x2 - mergedLine->x1);

			float angleDiff = abs(angle2 - angle1);
			float flippedDiff = (B_PI*2) - angleDiff;
			bool matchingAngle = angleDiff <= angleThreshold;

			int endpointEquals = areEndpointsEqual(currentLine, mergedLine);
			if (matchingAngle && endpointEquals != 0) {
				// Merge the lines and mark as merged
				// if flippedDiff, use flipped merge.
				mergeLines(currentLine, mergedLine, endpointEquals == 2);
				merged = true;
				break;
			}
		}

		if (!merged) {
			// add this line
			mergedLines.push_back(new CaveLine(currentLine->x1, currentLine->y1, currentLine->x2, currentLine->y2));
		}
	}

	return mergedLines;
}

// optimized for three vertices only
float LineOptimizer::calculateTriangleArea(std::vector<b2Vec2>& vertices) {

	float x1 = vertices[0].x;
	float y1 = vertices[0].y;
	float x2 = vertices[1].x;
	float y2 = vertices[1].y;
	float x3 = vertices[2].x;
	float y3 = vertices[2].y;

	return 0.5 * abs(x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
}

// calculates area of a closed polygon.
float LineOptimizer::calculatePolygonArea(std::vector<b2Vec2>& vertices) {
	int n = vertices.size();
	float area = 0.0;

	for (int i = 0; i < n; ++i) {
		int next = (i + 1) % n;
		area += (vertices[i].x * vertices[next].y - vertices[next].x * vertices[i].y);
	}

	area = std::abs(area) / 2.0;
	return area;
}

// Visvalingam–Whyatt algorithm
std::vector<CaveLine*> LineOptimizer::vwOptimization(std::vector<CaveLine*>& lines, float areaThreshold) {


	std::vector<b2Vec2> vertices;
	for (int i = 0; i < lines.size(); i++) {
		CaveLine* a = lines[i];
		vertices.push_back(b2Vec2(a->x1, a->y1));
		vertices.push_back(b2Vec2(a->x2, a->y2));
	}

	float totalArea = calculatePolygonArea(vertices);

	if (lines.size() < 6) {
		// if vertices are this small, it's likely too small to optimize any more without destroying its shape.
		return lines;
	}

	std::vector<CaveLine*> newLines;

	for (int i = 0; i < lines.size(); i++) {
		CaveLine* l1 = lines[(i + 0) % lines.size()];
		CaveLine* l2 = lines[(i + 1) % lines.size()];

		if (i + 1 == lines.size()) {
			// after looping around, connect to first newLine
			l2 = newLines[0];
		}

		b2Vec2 a = b2Vec2(l1->x1 + 0, l1->y1 + 0);
		b2Vec2 b = b2Vec2(l1->x2 + 0, l1->y2 + 0);
		b2Vec2 c = b2Vec2(l2->x2 + 0, l2->y2 + 0);

		std::vector<b2Vec2> verts { a, b, c };
		float area = calculateTriangleArea(verts);

		// check if b can be skipped without decreasing the total areaThreshold too much

		float actualThreshold = areaThreshold;
		if (area < actualThreshold) {
			// merge a to c, skip b

			CaveLine* newLine = new CaveLine(a.x, a.y, c.x, c.y);
			newLines.push_back(newLine);

			if (i + 1 == lines.size()) {
				// remove first line since it was optimized
				newLines.erase(newLines.begin());
			}

			i++;

		}
		else {
			// just add a-b
			CaveLine* newLine = new CaveLine(a.x, a.y, b.x, b.y);
			newLines.push_back(newLine);
		}

	}

	return newLines;
}

CaveOutline::CaveOutline(int seed2, float x2, float y2, float w, float h, float step, float modelStep, float borderModifier)
	: seed(seed2), borderX(x2), borderY(y2), width(w), height(h), step(step), modelStep(modelStep), borderModifier(borderModifier),
	lineHashing(w, h, width / 500) {
	stepWidth = ceil(w / step);
	stepHeight = ceil(h / step);

	validSpawningPositions = new unsigned char[stepWidth * stepHeight];
	buildBrush();
}

glm::vec3 CaveOutline::calculateHeightAndGradient(std::vector<float>& map, float x, float y) {
	// to positive coordinates in range of map
	float x0 = (x - borderX) / step;
	float y0 = (y - borderY) / step;

	// cell indexes
	int px = floor(x0);
	int py = floor(y0);

	// offset inside cell
	float fx = x0 - px;
	float fy = y0 - py;

	int max = stepWidth * stepHeight;
	int nodeIndexNW = px + py * stepWidth;
	float heightNW = map[nodeIndexNW];
	float heightNE = nodeIndexNW + 1 >= 0 && nodeIndexNW + 1 < max ? map[nodeIndexNW + 1] : 1;
	float heightSW = nodeIndexNW + stepWidth >= 0 && nodeIndexNW + stepWidth < max ? map[nodeIndexNW + stepWidth] : 1;
	float heightSE = nodeIndexNW + stepWidth + 1 >= 0 && nodeIndexNW + stepWidth + 1 < max ? map[nodeIndexNW + stepWidth + 1] : 1;

	float gradX = (heightNE - heightNW) * (1 - fy) + (heightSE - heightSW) * fy;
	float gradY = (heightSW - heightNW) * (1 - fx) + (heightSE - heightNE) * fx;

	float height = heightNW * (1 - fx) * (1 - fy) + heightNE * fx * (1 - fy) + heightSW * (1 - fx) * fy + heightSE * fx * fy;

	return glm::vec3(-gradX, -gradY, height);
}

void CaveOutline::buildBrush() {

	int brushLength = pow(brushRadius * 2 + 1, 2);
	brush.resize(brushLength);
	float brushSum = 0;
	int bi = 0;
	for (int i = -brushRadius; i <= brushRadius; i++) {
		for (int j = -brushRadius; j <= brushRadius; j++) {
			float dist = sqrt(i * i + j * j);

			float weight = 0;
			if (dist <= brushRadius) {
				weight = 1.0 - dist / brushRadius;
			}

			brushSum += weight;
			brush[bi++] = weight;
		}
	}
	// normalize brush
	for (int i = 0; i < brushLength; i++) {
		brush[i] = brush[i] / brushSum;
	}
}

void CaveOutline::erosion() {
	erosionCounts++;

	if (totalDroplets == 0) {
		return;
	}

	//std::cout << "Eroding terrain!" << std::endl;
	int w = stepWidth;
	int h = stepHeight;

	std::vector<float>& map = textureData; // use textureData

	int brushLength = pow(brushRadius * 2 + 1, 2);


	rndEngine.setSeed(1234 * erosionCounts);
	b2Vec2 windDir(cos(windAngle), sin(windAngle));
	bool useOpenMP = totalDroplets >= 10000;
	#pragma omp parallel for schedule(runtime) if(useOpenMP)
	for (int dropletIndex = 0; dropletIndex < totalDroplets; dropletIndex++) {

		glm::vec3 dropletPos(0, 0, 0); // in world coordinates
		dropletPos.x = borderX + rndEngine.randomFloat() * this->width;
		dropletPos.y = borderY + rndEngine.randomFloat() * this->height;
		//dropletPos.x = -20;
		//dropletPos.y = 0;

		b2Vec2 direction(0, 0);
		float sediment = 0.0f;
		float water = initialWaterVolume;
		float speed = initialSpeed;
		glm::vec3 heightAndGradient = calculateHeightAndGradient(map, dropletPos.x, dropletPos.y);

		for (int lifetime = 0; lifetime < maxDropletLifetime; lifetime++) {
			int x0 = (dropletPos.x - borderX) / step;
			int y0 = (dropletPos.y - borderY) / step;
			int di0 = x0 + y0 * w;

			// don't affect southern and easteren edges.
			if (x0 < 0 || x0 >= stepWidth - 1 || y0 < 0 || y0 >= stepHeight - 1) {
				break;
			}

			float wx = dropletPos.x - floor(dropletPos.x);
			float wy = dropletPos.y - floor(dropletPos.y);

			// calculate droplet height and flow
			dropletPos.z = heightAndGradient.z;
			glm::vec3 flowDir(heightAndGradient.x + windDir.x * windSpeed, heightAndGradient.y + windDir.y * windSpeed, 0);
			flowDir = glm::normalize(flowDir);


			if (lifetime == 0) {
				// set direction the same as flow on first tick
				direction.x = flowDir.x;
				direction.y = flowDir.y;

			}

			// update droplet position. interpolate between flow and velocity
			direction.x = direction.x * inertia + flowDir.x * (1 - inertia);
			direction.y = direction.y * inertia + flowDir.y * (1 - inertia);
			
			float dirLength = direction.Length();
			dropletPos.x += (direction.x / dirLength) * step / 2;
			dropletPos.y += (direction.y / dirLength) * step / 2;

			// stop if no flow or OOB
			bool shouldBreak = false;
			if ((direction.x == 0 && direction.y == 0) ||
				(dropletPos.x < borderX + 0.001 || dropletPos.x >= borderX + width + 0.001 || dropletPos.y < borderY + 0.001 || dropletPos.y >= borderY + height - 0.001)) {

				shouldBreak = true;
			}

			bool doStuff = true;

			if (shouldBreak) {
				break;
			}

			heightAndGradient = calculateHeightAndGradient(map, dropletPos.x, dropletPos.y);

			x0 = (dropletPos.x - borderX) / step;
			y0 = (dropletPos.y - borderY) / step;
			int dropIndex = x0 + y0 * w;

			// find droplets new height
			float newHeight = heightAndGradient.z;
			float deltaHeight = newHeight - dropletPos.z;


			// calculate sediment capacity
			float sedimentCapacity = std::max(-deltaHeight * speed * water * sedimentCapacityFactor, minSedimentCapacity);


			//if (dropletIndex < totalDroplets / 2) {
			//	bool willDeposit = sediment > sedimentCapacity || deltaHeight > 0;
			//	doStuff = false;
			//	map[di0] = willDeposit ? 1 : 0;
			//}

			//std::cout << std::fixed << std::setprecision(4);
			if (doStuff) {
				//std::cout << dropletIndex << ":" << lifetime << " xyz:" << dropletPos.x << "," << dropletPos.y << "," << dropletPos.z << " || h: " << newHeight << "  dh:" << deltaHeight << " sc:" << sedimentCapacityFactor;
				// if carrying too much sediment or going uphill 
				if (sediment > sedimentCapacity || deltaHeight > 0) {
					float amountToDeposit = deltaHeight > 0 ? std::min(deltaHeight, sediment) : (sediment - sedimentCapacity) * depositSpeed;
					sediment -= amountToDeposit;

					//std::cout << " deposit: " << amountToDeposit << endl;

					// deposit nearby

					if (doDeposition) {
						int index = 0;
						for (int i = -brushRadius; i <= brushRadius; i++) {
							for (int j = -brushRadius; j <= brushRadius; j++) {
								float weight = brush[index++];
								float weighted = weight * amountToDeposit;

								int d = di0 + i + j * w;

								if (d >= 0 && d < w * h)
									map[d] += weighted * (1 - wx) * (1 - wy);
								if (d + 1 >= 0 && d + 1 < w * h)
									map[d + 1] += weighted * wx * (1 - wy);
								if (d + w >= 0 && d + w < w * h)
									map[d + w] += weighted * (1 - wx) * wy;
								if (d + w + 1 >= 0 && d + w + 1 < w * h)
									map[d + w + 1] += weighted * wx * wy;
							}
						}
					}

				}
				// otherwise, erode
				else {
					//
					float hardness = 1.0f; // terrain is harder to erode as elevation increases

					if (map[di0] < threshold) {
						hardness = sandHardness; // sand
					}
					else {
						// rockier, erode slightly less
						float f = (map[di0] - threshold) / (1 - threshold);
						hardness = stoneHardness * f + sandHardness * (1.0 - f);
					}

					float amountToErode = std::max((sedimentCapacity - sediment) * erodeSpeed * water * hardness, -deltaHeight);

					float deltaGained = 0;
					// use brush to erode 
					int index = 0;
					for (int i = -brushRadius; i <= brushRadius; i++) {
						for (int j = -brushRadius; j <= brushRadius; j++) {
							float weight = brush[index++];

							if (weight > 0) {
								float weightErodeAmount = amountToErode * weight;

								int xv = (dropletPos.x - borderX) / step + i;
								int yv = (dropletPos.y - borderY) / step + j;
								int ind = xv + yv * w;
								if (ind >= 0 && ind < w * h) {

									float heightAt = map[ind];
									float deltaSediment = heightAt < weightErodeAmount ? heightAt : weightErodeAmount; // dont erode below 0

									if (doErosion) {
										map[ind] -= deltaSediment;
									}
									deltaGained += deltaSediment;
								}
							}
						}
					}
					sediment += deltaGained;
					//std::cout << " eroded: " << deltaGained << endl;
				}
			}


			speed = sqrt(speed * speed + deltaHeight * gravity);
			water *= 1 - evaporateSpeed;
		}
	}

	// map is a reference to textureData, and the data is stored there already.
}

bool CaveOutline::isInWalls(float x, float y) {
	float val = getNoiseLevel(x, y);

	// fast check if it is firmly inside walls.
	if (val >= threshold) {
		return true;
	}


	int x0 = (x - borderX) / step;
	int y0 = (y - borderY) / step;

	// check if it is inside any hitbox
	bool inAnyBoxes = false;
	for (RectangleBorders* rect : chainBoxes) {
		if (rect->x <= x && rect->width > x && rect->y <= y && rect->height > y) {
			inAnyBoxes = true;
			break;
		}
	}

	if (!inAnyBoxes) {
		// if not inside any hitbox, it cant be inside a chain.
		return false;
	}

	// check if position has already been checked
	unsigned char& cValue = validSpawningPositions[x0 + y0 * stepWidth];
	if (cValue != 0) {
		// already checked
		return cValue == 1; // if 1, it is inside the wall
	}


	// get nearest cave line. if we're facing the normal, then we're inside the wall.

	HashResult result;
	lineHashing.query(result, x, y, std::max(width, height) / 4, nullptr);

	int closestIndex = -1;
	int index = 0;
	for (float dist : result.distances) {

		if (closestIndex == -1 || dist < result.distances[closestIndex])
			closestIndex = index;

		index++;
	}
	if (closestIndex == -1) {
		return true; // not near a line, just say yes.
	}

	CaveLine* closestLine = static_cast<CaveLine*>(result.nearby[closestIndex]);
	float dx = closestLine->x2 - closestLine->x1;
	float dy = closestLine->y2 - closestLine->y1;
	b2Vec2 lineNormal = b2Vec2(-dy, dx);
	float sum = lineNormal.x + lineNormal.y;
	if (sum == 0)
		sum = 1;
	lineNormal.x /= sum;
	lineNormal.y /= sum;

	float vx = x - closestLine->x1;
	float vy = y - closestLine->y1;
	float dotProduct = vx * lineNormal.x + vy * lineNormal.y;

	// true if inside wall
	bool output = dotProduct < 0;

	// mark as already discovered
	validSpawningPositions[x0 + y0 * stepWidth] = output ? 1 : 2;
	
	return output;
}

void CaveOutline::buildShape() {
	float step = modelStep;

		// make a simple square for triangles.
	unsigned int ind[] = {
		0, 1, 2,
		3, 4, 5
	};
	float vertices[] = {
		borderX, borderY,
		borderX, borderY + height,
		borderX + width, borderY + height,

		borderX, borderY,
		borderX + width, borderY,
		borderX + width, borderY + height,
	};

	for (float v : vertices) {
		flatTriangles.push_back(v);
	}
	for (unsigned int v : ind) {
		flatIndices.push_back(v);
	}

	//if (flatIndices.size() > 0) {
	//	return; // disable other triangles for now.
	//}

	// build triangles and indices
	indices.clear();
	triangles.clear();

	float stepWidth = width / step;
	float stepHeight = height / step;
	triangles.resize(3 * (stepWidth + 1) * (stepHeight + 1));

	int index = 0;
	for (int y = 0; y <= stepHeight; y++) {
		for (int x = 0; x <= stepWidth; x++) {
			float x0 = (borderX + x * step);
			float y0 = (borderY + y * step);
			float z0 = getNoiseLevel(x0, y0) * renderHeightScale;
			int i0 = x + y * stepWidth;

			// add this vertex
			triangles[index++] = x0;
			triangles[index++] = y0;
			triangles[index++] = z0;
		}
	}

	indices.resize(6 * stepWidth * stepHeight);
	index = 0;
	for (int y = 0; y < stepHeight; y++) {
		for (int x = 0; x < stepWidth; x++) {

			float x0 = (borderX + x * step);
			float y0 = (borderY + y * step);
			float z0 = getNoiseLevel(x0, y0);
			int i0 = x + y * stepWidth;

			float x1 = (borderX + x * step) + step;
			float y1 = (borderY + y * step);
			float z1 = getNoiseLevel(x0, y0);
			int i1 = x + 1 + y * stepWidth;

			float x2 = (borderX + x * step) + step;
			float y2 = (borderY + y * step) + step;
			float z2 = getNoiseLevel(x0, y0);
			int i2 = x + 1 + (y+1) * stepWidth;

			float x3 = (borderX + x * step);
			float y3 = (borderY + y * step) + step;
			float z3 = getNoiseLevel(x0, y0);
			int i3 = x + (y+1) * stepWidth;

			// add indices to connect vertexes (clockwise)
			indices[index++] = i0;
			indices[index++] = i1;
			indices[index++] = i2;

			indices[index++] = i0;
			indices[index++] = i2;
			indices[index++] = i3;
		}
	}

	indicesCount = indices.size();
	std::cout << "Mesh triangles: " << triangles.size() / 3 << std::endl;

}

int CaveOutline::toStepIndex(float x, float y) {
	int x0 = (x - borderX) / step;
	int y0 = (y - borderY) / step;
	return x0 + y0 * stepWidth;
}

float CaveOutline::getNoiseLevel(float x0, float y0) {
	int x = (x0 - borderX) / step;
	int y = (y0 - borderY) / step;

	if (x < 1 || x >= stepWidth-1) {
		return 0;
	}
	if (y < 1 || y >= stepHeight-1) {
		return 0;
	}


	int ind = x + y * stepWidth;

	// get from texture
	float n = originalTextureData[ind];


	return n;
}

void CaveOutline::build() {
	lines.clear();

	// use marching square algorithm to generate lines.
	// marching square algorithm

	// when generating the border lines, less precision is needed. borderModifier alters how much the step is
	float step = this->step * borderModifier;

	int stepWidth = floor(width / step);
	int stepHeight = floor(height / step);
	float hs = step / 2;
	std::cout << "Building caves of size: " << stepWidth << "x" << stepHeight << std::endl;

	b2Vec2 halfStep = b2Vec2(step / 2, -step / 2);
	b2Vec2 bottomLeft = b2Vec2(borderX, borderY) + halfStep;

	int totalSteps = stepWidth * stepHeight;
	int currentStep = 0;

	std::cout << "Building caves";
	for (int x = -1; x <= stepWidth; x++) {
		for (int y = -1; y <= stepHeight; y++) {
			if (currentStep % totalSteps / 2 == 0) {
				std::cout << ".";
			}
			currentStep++;

			float x0 = (borderX + x * step);
			float y0 = (borderY + y * step);
			// divide map into 2x2 pixel regions
			float v1 = getNoiseLevel(x0, y0 - step); // top left
			float v2 = getNoiseLevel(x0 + step, y0 - step); // top right
			float v3 = getNoiseLevel(x0 + step, y0); // bottom right
			float v4 = getNoiseLevel(x0, y0); // bottom left

			b2Vec2 a = b2Vec2(x * step + hs, y * step) + bottomLeft;
			b2Vec2 b = b2Vec2(x * step + step, y * step + hs) + bottomLeft;
			b2Vec2 c = b2Vec2(x * step + hs, y * step + step) + bottomLeft;
			b2Vec2 d = b2Vec2(x * step, y * step + hs) + bottomLeft;

			// fli
			int state = getState(v4 >= threshold, v3 >= threshold, v2 >= threshold, v1 >= threshold);

			switch (state) {
			case 0: {
				break;
			}
			case 1: {
				lines.push_back(lineFrom(a, d));
				break;
			}
			case 2: {
				lines.push_back(lineFrom(b, a));
				break;
			}
			case 3: {
				lines.push_back(lineFrom(b, d));
				break;
			}
			case 4: {
				lines.push_back(lineFrom(c, b));
				break;
			}
			case 5: {
				lines.push_back(lineFrom(c, d));
				lines.push_back(lineFrom(a, b));
				break;
			}
			case 6: {
				lines.push_back(lineFrom(c, a));
				break;
			}
			case 7: {
				lines.push_back(lineFrom(c, d));
				break;
			}
			case 8: {
				lines.push_back(lineFrom(d, c));
				break;
			}
			case 9: {
				lines.push_back(lineFrom(a, c));
				break;
			}
			case 10: {
				lines.push_back(lineFrom(b, c));
				lines.push_back(lineFrom(d, a));
				break;
			}
			case 11: {
				lines.push_back(lineFrom(b, c));
				break;
			}
			case 12: {
				lines.push_back(lineFrom(d, b));
				break;
			}
			case 13: {
				lines.push_back(lineFrom(a, b));
				break;
			}
			case 14: {
				lines.push_back(lineFrom(d, a));
				break;
			}
			case 15: {
				break;
			}

			default:
				break;
			}

		}
	}
	std::cout << " Done!" << std::endl;

	buildChain();

	// compress lines
	int startingSize = 0;
	for (std::vector<CaveLine*>& list : chains) {
		startingSize += list.size();
	}

	std::vector<CaveLine*> results = lines;
	int ls = lines.size();
	std::cout << "Compressing: ";
	for (int i = 0; i < compressionIterations; i++) {


		int es = 0;
		for (std::vector<CaveLine*>& list : chains) {
			list = LineOptimizer::vwOptimization(list, areaThreshold);
			es += list.size();
		}


		std::cout << std::fixed << std::setprecision(2);
		std::cout << "-" << (ls - es) << ", ";

		ls = es;
	}
	std::cout << "Compressed by " << (1.0 - float(ls) / startingSize) * 100 << "%" << std::endl;
	std::cout << "Total lines: " << startingSize << "->" << ls << std::endl;

	lines.clear();
	lineHashing.clear();
	for (std::vector<CaveLine*>& list : chains) {
		for (CaveLine* cl : list) {
			lines.push_back(cl);
			lineHashing.insert(cl);
		}
	}


	buildChain();
	int loopingChains = 0;

	for (std::vector<CaveLine*>& list : chains) {
		int e = list.size() - 1;
		bool loops = abs(list[0]->x1 - list[e]->x2) < 0.001 && abs(list[0]->y1 - list[e]->y2) < 0.001;

		if (loops) {
			loopingChains++;
		}
	}


	std::cout << "Total chains: " << chains.size() << ", Loops: " << loopingChains << std::endl;
}

void CaveOutline::buildChain() {
	// get chains of lines
	chains.clear();
	for (CaveLine* line : lines) {

		bool isFront = false;
		bool onChain = false;
		int chainIndex = 0;
		for (int i = 0; i < chains.size(); i++) {
			std::vector<CaveLine*>& list = chains[i];
			// no list will ever be empty

			CaveLine* start = list[0];

			// if end of line connects to start of Start line.
			if (abs(start->x1 - line->x2) < 0.001 && abs(start->y1 - line->y2) < 0.001) {
				onChain = true;
				isFront = true;
				chainIndex = i;
				break;
			}

			CaveLine* end = list[list.size() - 1]; // intentionally gets first line if size=1
			// if end of last line connects to start of this line.
			if (abs(end->x2 - line->x1) < 0.001 && abs(end->y2 - line->y1) < 0.001) {
				onChain = true;
				isFront = false;
				chainIndex = i;
				break;
			}


		}

		if (onChain) {
			std::vector<CaveLine*>& list = chains[chainIndex];

			if (isFront) {
				list.insert(list.begin(), line);
			}
			else {
				list.push_back(line);
			}
		}
		else {
			// create new chain

			std::vector<CaveLine*> list;
			list.push_back(line);
			chains.push_back(list);
		}

	}

	// combine chains that connect

	std::vector<bool> ignoredChains(chains.size(), false);
	for (int i = 0; i < chains.size(); i++) {
		std::vector<CaveLine*>& list = chains[i];
		if (list.size() == 0) {
			// list has been added elsewhere, ignore.
			continue;
		}
		int e = list.size() - 1;
		bool loops = abs(list[0]->x1 - list[e]->x2) < 0.001 && abs(list[0]->y1 - list[e]->y2) < 0.001;

		if (!loops) {

			// if it doesnt loop, check if it can connect to another line.

			for (int j = 0; j < chains.size(); j++) {
				std::vector<CaveLine*>& l2 = chains[j];

				// find a non-empty, non-looping, non-self chain that connects to this chain.
				// clear that chain and continue looking. After finding a chain, repeat the loop to check if previous chains can now connect.


				if (l2.size() == 0)
					continue;

				if (list != l2) {
					int e2 = l2.size() - 1;
					bool loops2 = abs(l2[0]->x1 - l2[e2]->x2) < 0.001 && abs(l2[0]->y1 - l2[e2]->y2) < 0.001;

					if (!loops2) {
						// ignore self list, empty lists, and looped lists.
						// this chain should be ready to check.

						if (LineOptimizer::areEndpointsEqual(list[e], l2[0])) {
							// end of this chain connects to start of that chain.

							// insert l2 at end
							for (CaveLine* line2 : l2) {
								list.push_back(line2);
							}
							l2.clear();

						}
						else if (LineOptimizer::areEndpointsEqual(list[0], l2[l2.size() - 1])) {
							// start of this chain connects to end of that chain

							// insert l2 at start
							auto listStart = list.begin();
							int index = 0;
							// add list A to end of list B
							for (CaveLine* cl : list) {
								l2.push_back(cl);
								index++;
							}
							list.clear(); // list list A

							// add all of list B to list A
							for (CaveLine* line2 : l2) {
								list.push_back(line2);
							}
							l2.clear();
						}
						else {
							continue;
						}

						// ignore the used chain since it is now included in another chain.
						ignoredChains[j] = true;

						// at this point, we reset the j for-loop
						j = -1;
						continue;
					}

				}
			}


		}

	}

	std::vector<std::vector<CaveLine*>> newChains;
	for (int i = 0; i < chains.size(); i++) {
		if (!ignoredChains[i]) {
			std::vector<CaveLine*>& list = chains[i];
			int e = list.size() - 1;
			bool loops = abs(list[0]->x1 - list[e]->x2) < 0.001 && abs(list[0]->y1 - list[e]->y2) < 0.001;

			if (!loops) {
				// make it loop
				//list.push_back(new CaveLine{ list[e]->x2, list[e]->y2, list[0]->x1, list[0]->y1 });
			}
			newChains.push_back(list);
		}
	}
	chains = newChains;

	chainBoxes.clear();
	for (std::vector<CaveLine*> list : chains) {
		RectangleBorders* rect = new RectangleBorders(10000000, 1000000000, -10000000, -1000000);
		
		for (CaveLine* line : list) {
			if (rect->x < line->x1) {
				rect->x = line->x1;
			}
			if (rect->width > line->x1) {
				rect->width = line->x1;
			}

			if (rect->y < line->y1) {
				rect->y = line->y1;
			}
			if (rect->height > line->y1) {
				rect->height = line->y1;
			}
		}

		chainBoxes.push_back(rect);

	}

}

CaveLine* CaveOutline::lineFrom(b2Vec2 a, b2Vec2 b) {
	CaveLine* line = new CaveLine( a.x, a.y, b.x, b.y);
	return line;
}

int CaveOutline::getState(bool a, bool b, bool c, bool d) {
	int a0 = a ? 1 : 0;
	int b0 = b ? 1 : 0;
	int c0 = c ? 1 : 0;
	int d0 = d ? 1 : 0;

	// transform into bits
	return (a0 * 8) + (b0 * 4) + (c0 * 2) + d0;
}