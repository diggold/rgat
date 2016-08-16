#include "stdafx.h"
#include "rendering.h"

//draw an outline sphere of size diam
//we could just memset the colours array but leaving it here for the sake of adapatbility
void plot_wireframe(VISSTATE *clientstate)
{
	int ii, pp, index;
	int diam = clientstate->activeGraph->m_scalefactors->radius;
	int points = POINTSPERLINE;
	int numSphereCurves = 0;
	int lineDivisions = (int)(360 / WIREFRAMELOOPS);
	GRAPH_DISPLAY_DATA *wireframe_data = clientstate->wireframe_sphere;

	GLfloat *vpos = wireframe_data->acquire_pos();
	GLfloat *vcol = wireframe_data->acquire_col();
	for (ii = 0; ii < 180; ii += lineDivisions) {

		float ringSize = diam * sin((ii*M_PI) / 180);
		for (pp = 0; pp < POINTSPERLINE; pp++) {

			float angle = (2 * M_PI * pp) / POINTSPERLINE;

			index = numSphereCurves * POINTSPERLINE * POSELEMS + pp * POSELEMS;
			vpos[index] = ringSize * cos(angle);
			vpos[index + 1] = diam * cos((ii*M_PI) / 180);
			vpos[index + 2] = ringSize * sin(angle);

			index = numSphereCurves * POINTSPERLINE * COLELEMS + pp * COLELEMS;
			vcol[index] = wireframe_col.r;
			vcol[index + 1] = wireframe_col.g;
			vcol[index + 2] = wireframe_col.b;
			vcol[index + 3] = wireframe_col.a;
		}
		numSphereCurves += 1;
	}

	for (ii = 0; ii < 180; ii += lineDivisions) {

		float degs2 = (ii*M_PI) / 180;  
		for (pp = 0; pp < points; pp++) {

			float angle = (2 * M_PI * pp) / points;
			float cosangle = cos(angle);

			index = numSphereCurves * POINTSPERLINE * POSELEMS + pp * POSELEMS;
			vpos[index] = diam * cosangle * cos(degs2);
			vpos[index + 1] = diam * sin(angle);
			vpos[index + 2] = diam * cosangle * sin(degs2);

			index = numSphereCurves * POINTSPERLINE * COLELEMS + pp * COLELEMS;
			vcol[index] = wireframe_col.r;
			vcol[index + 1] = wireframe_col.g;
			vcol[index + 2] = wireframe_col.b;
			vcol[index + 3] = wireframe_col.a;
		}
		numSphereCurves += 1;
	}

	glGenBuffers(2, clientstate->wireframeVBOs);
	load_VBO(VBO_SPHERE_POS, clientstate->wireframeVBOs, WFPOSBUFSIZE, vpos);
	load_VBO(VBO_SPHERE_COL, clientstate->wireframeVBOs, WFCOLBUFSIZE, vcol);
	wireframe_data->release_pos();
	wireframe_data->release_col();
}

void drawShortLinePoints(FCOORD *startC, FCOORD *endC, ALLEGRO_COLOR *colour, GRAPH_DISPLAY_DATA *vertdata, int *arraypos)
{

	GLfloat* vertpos = vertdata->acquire_pos();
	GLfloat* vertcol = vertdata->acquire_col();

	int numverts = vertdata->get_numVerts();
	int posi = numverts * POSELEMS;
	int coli = numverts * COLELEMS;
	*arraypos = coli;

	memcpy(vertpos + posi, startC, POSELEMS * sizeof(float));
	posi += POSELEMS;
	memcpy(vertpos + posi, endC, POSELEMS * sizeof(float));

	memcpy(vertcol + coli, colour, COLELEMS * sizeof(float));
	coli += COLELEMS;
	memcpy(vertcol + coli, colour, COLELEMS * sizeof(float));

	vertdata->set_numVerts(numverts + 2);
	vertdata->release_pos();
	vertdata->release_col();

}

int drawLongCurvePoints(FCOORD *bezierC, FCOORD *startC, FCOORD *endC, ALLEGRO_COLOR *colour, 
	int edgeType, GRAPH_DISPLAY_DATA *vertdata, int curvePoints, int *arraypos) {
	float fadeArray[] = { 1,0.9,0.8,0.7,0.5,0.3,0.3,0.3,0.2,0.2,0.2,
		0.3, 0.3, 0.5, 0.7, 0.9, 1 };
	
	curvePoints += 2;
	float *posdata = (float *)malloc((curvePoints + 2) * 3 * sizeof(float));
	float *coldata = (float *)malloc((curvePoints + 2) * 4 * sizeof(float));
	if (!posdata || !coldata) return 0;
	int ci = 0;
	int pi = 0;

	float r = colour->r;
	float g = colour->g;
	float b = colour->b;

	posdata[pi++] = startC->x;
	posdata[pi++] = startC->y;
	posdata[pi++] = startC->z;
	coldata[ci++] = r;
	coldata[ci++] = g;
	coldata[ci++] = b;
	coldata[ci++] = 1;

	// > for smoother lines, less performance
	int dt;
	float fadeA = 0.9;
	FCOORD resultC;

	int segments = float(curvePoints) / 2;
	for (dt = 1; dt < segments + 1; dt++)
	{

		bezierPT(startC, bezierC, endC, dt, segments, &resultC);

		//end last line
		posdata[pi++] = resultC.x;
		posdata[pi++] = resultC.y;
		posdata[pi++] = resultC.z;
		//start new line at same point todo: this is waste of memory
		posdata[pi++] = resultC.x;
		posdata[pi++] = resultC.y;
		posdata[pi++] = resultC.z;

		if ((edgeType == IOLD) || (edgeType == IRET)) {
			fadeA = fadeArray[dt - 1];
			if (fadeA > 1) fadeA = 1;
		}
		else
			fadeA = 0.9;

		coldata[ci++] = r;
		coldata[ci++] = g;
		coldata[ci++] = b;
		coldata[ci++] = fadeA;
		coldata[ci++] = r;
		coldata[ci++] = g;
		coldata[ci++] = b;
		coldata[ci++] = fadeA;
	}

	posdata[pi++] = endC->x;
	posdata[pi++] = endC->y;
	posdata[pi++] = endC->z;

	coldata[ci++] = r;
	coldata[ci++] = g;
	coldata[ci++] = b;
	coldata[ci++] = 1;

	int numverts = vertdata->get_numVerts();
	float *vpos = vertdata->acquire_pos() + numverts * POSELEMS;
	float *vcol = vertdata->acquire_col() + numverts * COLELEMS;
	*arraypos = numverts * COLELEMS;

	memcpy(vpos, posdata, POSELEMS * curvePoints * sizeof(float));
	memcpy(vcol, coldata, COLELEMS * curvePoints * sizeof(float));

	free(posdata);
	free(coldata);

	vertdata->set_numVerts(numverts + curvePoints + 2);
	vertdata->release_col();
	vertdata->release_pos();
	return curvePoints + 2;
}

//connect two nodes with an edge of automatic number of vertices
int drawCurve(GRAPH_DISPLAY_DATA *linedata, FCOORD *startC, FCOORD *endC, 
	ALLEGRO_COLOR *colour, int edgeType, MULTIPLIERS *dimensions, int *arraypos)
{
	float r, b, g, a;
	r = colour->r;
	b = colour->b;
	g = colour->g;
	a = 1;

	// describe the normal
	FCOORD middleC;
	midpoint(startC, endC, &middleC);
	float eLen = distance(startC, endC);

	FCOORD *bezierC;
	int curvePoints;

	switch (edgeType)
	{
		case INEW:
		{
			//todo: this number depends on the scale!
			curvePoints = eLen < 80 ? 1 : LONGCURVEPTS;
			bezierC = &middleC;
			break;
		}
		case ICALL:
		{
			curvePoints = LONGCURVEPTS;
			bezierC = &middleC;
			break;
		}

		case IRET:
		case IOLD:
		{
			//no idea what this does
			float a = BACKVERTA;
			curvePoints = LONGCURVEPTS;
			// divergence = -random.uniform(eLen / 5, eLen / 2)
			if (eLen < 2) 
				bezierC = &middleC;
			else
			{
				float oldMidA, oldMidB;
				FCOORD bezierC2;
				bezierC = &bezierC2;
				sphereAB(&middleC, &oldMidA, &oldMidB, dimensions);
				sphereCoord(oldMidA, oldMidB, bezierC, dimensions, -(eLen / 2));

				// i dont know why this maths problem happens or why this fixes it
				// but at this point i'm too afraid to ask.
				if ((bezierC->x > 0) && (startC->x < 0 && endC->x < 0))
					bezierC->x = -bezierC->x;
			}
			break;
		}

		case ILIB: 
		{
			curvePoints = LONGCURVEPTS;
			bezierC = &middleC;
			break;
		}

		default:
			printf("\t\t!!!unknown colour\n");
			return 0;
	}

	switch(curvePoints)
	{
		case LONGCURVEPTS:
		{
			int vertsdrawn = drawLongCurvePoints(bezierC, startC, endC, colour, edgeType, linedata, curvePoints, arraypos);
			return vertsdrawn; 
		}

		case 1:
			drawShortLinePoints(startC, endC, colour, linedata, arraypos);
			return 2;

		default:
			printf("ERROR: unknown curvepoints %d\n", curvePoints);
	}

	return curvePoints;
}



int add_vert(node_data *n, GRAPH_DISPLAY_DATA *vertdata, GRAPH_DISPLAY_DATA *animvertdata, MULTIPLIERS *dimensions)
{
	ALLEGRO_COLOR *active_col;

	float adjB = n->vcoord.b + float(n->vcoord.bMod * BMODMAG);
	FCOORD screenc;
	sphereCoord(n->vcoord.a, adjB, &screenc, dimensions, 0);

	int vertIdx = n->index;

	GLfloat *vpos = vertdata->acquire_pos();
	vpos[(vertIdx * POSELEMS)] = screenc.x;
	vpos[(vertIdx * POSELEMS) + 1] = screenc.y;
	vpos[(vertIdx * POSELEMS) + 2] = screenc.z;

	//todo: why do they have the same stuff in...?
	GLfloat *fcoord = vertdata->acquire_fcoord();
	fcoord[(vertIdx * POSELEMS)] = screenc.x;
	fcoord[(vertIdx * POSELEMS) + 1] = screenc.y;
	fcoord[(vertIdx * POSELEMS) + 2] = screenc.z;


	//todo: find better way, esp for custom colours
	if (n->external)
		active_col = &al_col_green;
	else {
		switch (n->ins->itype) 
		{
			case OPUNDEF:
				if (n->conditional == NOTCONDITIONAL)
					active_col = &al_col_yellow;
				else 
					active_col = &al_col_red;
				break;
			case OPJMP:
				active_col = &al_col_red;
				break;
			case OPRET:
				active_col = &al_col_orange;
				break;
			case OPCALL:
				active_col = &al_col_purple;
				break;

			case ISYS: //todo: never used
				active_col = &al_col_grey;
				break;

			default:
				printf("ERROR: Unhandled add_Vert color: %c\n", n->ins->itype);
				return 0;
		}
	}

	GLfloat *vcol = vertdata->acquire_col();
	vcol[(vertIdx * COLELEMS)] = active_col->r;
	vcol[(vertIdx * COLELEMS) + 1] = active_col->g;
	vcol[(vertIdx * COLELEMS) + 2] = active_col->b;
	vcol[(vertIdx * COLELEMS) + 3] = 1;

	vertdata->set_numVerts(vertdata->get_numVerts()+1);
	vertdata->release_col();
	vertdata->release_pos();
	vertdata->release_fcoord();

	GLfloat *vcol2 = animvertdata->acquire_col();
	vcol2[(vertIdx * COLELEMS)] = active_col->r;
	vcol2[(vertIdx * COLELEMS) + 1] = active_col->g;
	vcol2[(vertIdx * COLELEMS) + 2] = active_col->b;
	vcol2[(vertIdx * COLELEMS) + 3] = 0;

	animvertdata->set_numVerts(vertdata->get_numVerts() + 1);
	animvertdata->release_col();

	return 1;
}

int draw_new_verts(thread_graph_data *graph, GRAPH_DISPLAY_DATA *vertsdata) {
	
	MULTIPLIERS *scalefactors = vertsdata->isPreview() ? graph->p_scalefactors : graph->m_scalefactors;

	map<unsigned int, node_data>::iterator vertit = graph->get_vertStart();
	map<unsigned int, node_data>::iterator vertEnd = graph->get_vertEnd();
	if (vertit == vertEnd) return 0;
	std::advance(vertit, vertsdata->get_numVerts());

	if (vertit == vertEnd) return 0;

	printf("NEW VERTS (tid:%d)! Drawing....\n",graph->tid);
	for (; vertit != vertEnd; ++vertit)
	{
	 if (!add_vert(&vertit->second, vertsdata, graph->animvertsdata, scalefactors))
		{
			printf("Exiting with error in add vert\n");
			return -1;
		}
	}

	return 1;
}


//resize all drawn verts to new diameter
void resize_verts(thread_graph_data *graph, GRAPH_DISPLAY_DATA *vertsdata) {

	MULTIPLIERS *scalefactors = vertsdata->isPreview() ? graph->p_scalefactors : graph->m_scalefactors;

	map<unsigned int, node_data>::iterator vertit = graph->get_vertStart();
	map<unsigned int, node_data>::iterator target = graph->get_vertStart();

	GLfloat *vpos = vertsdata->acquire_pos();
	GLfloat *fcoord = vertsdata->acquire_fcoord();
	for (std::advance(target, vertsdata->get_numVerts()); vertit != target; vertit++)
	{
		FCOORD c = vertit->second.sphereCoordB(scalefactors, 0);
		int vertIdx = vertit->second.index;
		vpos[(vertIdx * POSELEMS)] = c.x;
		vpos[(vertIdx * POSELEMS) + 1] = c.y;
		vpos[(vertIdx * POSELEMS) + 2] = c.z;
		fcoord[(vertIdx * POSELEMS)] = c.x;
		fcoord[(vertIdx * POSELEMS) + 1] = c.y;
		fcoord[(vertIdx * POSELEMS) + 2] = c.z;
	}
	vertsdata->release_pos();
	vertsdata->release_fcoord();
}

int render_main_graph(VISSTATE *clientState)
{
	
	int adjustedDiam;
	bool doResize = false;

	thread_graph_data *graph = (thread_graph_data*)clientState->activeGraph;
	printf("%d,", graph->get_mainverts()->get_numVerts());
	adjustedDiam = graph->maxA * 10;
	adjustedDiam = max(adjustedDiam, 30000)* graph->m_scalefactors->userDiamModifier;
	recalculate_scale(graph->m_scalefactors, adjustedDiam);
	graph->m_scalefactors->radius = adjustedDiam;

	if (!obtainMutex(graph->edMutex, "Render Main Graph")) return 0;
	
	if (abs(adjustedDiam - graph->zoomLevel) > 5000 || clientState->rescale)
	{
		doResize = true;
		clientState->rescale = false;
	}
	
	if (doResize)
	{
		resize_verts(graph, graph->get_mainverts());
		graph->zoomLevel = graph->m_scalefactors->radius;
		graph->needVBOReload_main = true;
	}

	int drawCount = draw_new_verts(graph, graph->get_mainverts());
	if (drawCount < 0)
	{
		ReleaseMutex(graph->edMutex);
		printf("\n\nFATAL 5: Failed drawing verts!\n\n");
		return 0;
	}
	if (drawCount)
		graph->needVBOReload_main = true;


	//draw edges
	GRAPH_DISPLAY_DATA *lines = graph->get_mainlines();
	vector<pair<unsigned int, unsigned int>>::iterator edgeIt;

	if (doResize)
	{
		printf("resetting mainlines for resize\n");
		graph->reset_mainlines();
		lines = graph->get_mainlines();
		edgeIt = graph->edgeList.begin();
	}
	else
	{
		edgeIt = graph->edgeList.begin();
		std::advance(edgeIt, lines->get_renderedEdges());
	}

	if (edgeIt != graph->edgeList.end())
		graph->needVBOReload_main = true;

	for (; edgeIt != graph->edgeList.end(); ++edgeIt)
	{
		graph->render_edge(*edgeIt, lines, &clientState->guidata->lineColoursArr);
		graph->extend_faded_edges();
		lines->inc_edgesRendered();
	}
	ReleaseMutex(graph->edMutex);
	return 1;
}

int draw_new_preview_edges(VISSTATE* clientstate, thread_graph_data *graph)
{
	//draw edges
	vector<pair<unsigned int, unsigned int>>::iterator edgeIt = graph->edgeList.begin();
	std::advance(edgeIt, graph->previewlines->get_renderedEdges());
	if (edgeIt != graph->edgeList.end())
		graph->needVBOReload_preview = true;

	for (; edgeIt != graph->edgeList.end(); ++edgeIt)
	{
		if (!graph->render_edge(*edgeIt, graph->previewlines, &clientstate->guidata->lineColoursArr, 0, true))
		{
			printf("ERROR: Failed to add edge to preview graph\n");
			return 0; //todo make error -1 and give it name
		}
		graph->previewlines->inc_edgesRendered();
	}

	return 1;
}

int render_preview_graph(thread_graph_data *previewGraph, bool *rescale, VISSTATE *clientState)
{
	int adjustedDiam;
	bool doResize = false;
	
	if (!obtainMutex(previewGraph->edMutex, "Render Preview Graph")) return 0;

	adjustedDiam = previewGraph->maxA * 10;
	previewGraph->needVBOReload_preview = true;

	int vresult = draw_new_verts(previewGraph, previewGraph->previewverts);
	if (vresult == -1)
	{
		printf("\n\nFATAL 5: Failed drawing new verts! returned:%d\n\n", vresult);
		return 0;
	}

	vresult = draw_new_preview_edges(clientState, previewGraph);
	if (!vresult)
	{
		printf("\n\nFATAL 6: Failed drawing new edges! returned:%d\n\n", vresult);
		return 0;
	}
	ReleaseMutex(previewGraph->edMutex);
	return 1;
}


void draw_func_args(VISSTATE *clientstate, thread_graph_data *graph, ALLEGRO_FONT *font, DCOORD screenCoord, int externi, node_data *n)
{
	//todo: obviously needs lots of cleanup
	vector<string> callings;
	int calli = 0;
	int argtrack = -1;

	stringstream argstring;
	vector<vector<pair<int, string>>>::iterator callIt;

	argstring << n->nodeSym << "(";
	obtainMutex(graph->callArgsMutex, "Locking call args", 4000);
	//todo: this iterator has a race and will crash
	for (callIt = n->funcargs.begin(); callIt != n->funcargs.end(); callIt++)
	{
		vector<pair<int, string>>::iterator argIt;
		for (argIt = callIt->begin(); argIt != callIt->end(); argIt++)
		{
			if (argIt->first <= argtrack)
			{
				argstring << ")";
				callings.push_back(argstring.str());
				argstring.str("");
				argstring.clear();
				argtrack = -1;
				argstring << n->nodeSym << "(";

			}
			argtrack = argIt->first;
			argstring << argtrack << ":" << argIt->second << ',';
		}
	}
	ReleaseMutex(graph->callArgsMutex);

	if (argtrack != -1)
	{
		argstring << ")";
		callings.push_back(argstring.str());
		argstring.str("");
		argstring.clear();
		argtrack = -1;
		argstring << n->nodeSym << "(";

	}
	
	argstring.clear();
	argtrack = -1;
	int cidx = 0;

	
	vector<string>::iterator callit;
	for (callit = callings.begin(); callit != callings.end(); callit++)
	{
		if (cidx > 4) break;
		al_draw_text(font, al_col_white, screenCoord.x + INS_X_OFF,
			clientstate->size.height - screenCoord.y + INS_Y_OFF*++cidx, ALLEGRO_ALIGN_LEFT,
			callit->c_str());

	}

	if (!cidx)
	{
		argstring.str("");
		argstring.clear();
		argstring << n->index << ":" << n->nodeSym << "(...)";
		al_draw_text(font, al_col_white, screenCoord.x + INS_X_OFF,
			clientstate->size.height - screenCoord.y + INS_Y_OFF, ALLEGRO_ALIGN_LEFT,
			argstring.str().c_str());
	}
	
}

void show_extern_labels(VISSTATE *clientstate, PROJECTDATA *pd, thread_graph_data *graph, GRAPH_DISPLAY_DATA *vertdata)
{
	size_t externi;
	for (externi = 0; externi < graph->externList.size(); ++externi)
	{
		int externVertIdx = graph->externList[externi].first;
		node_data *n = graph->get_vert(externVertIdx);
		if (!n->nodeSym.size()) continue;

		DCOORD screenCoord = n->get_screen_pos(vertdata, pd);

		if (is_on_screen(&screenCoord, clientstate))
			draw_func_args(clientstate, graph, clientstate->standardFont, screenCoord, externi, n);
	}
}



void draw_instruction_text(VISSTATE *clientstate, int zdist, PROJECTDATA *pd, thread_graph_data *graph, GRAPH_DISPLAY_DATA *vertsdata)
{

	//iterate through nodes looking for ones that map to screen coords
	unsigned int i, drawn = 0;
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	bool show_all_always = (clientstate->show_ins_text == INSTEXT_ALL_ALWAYS);
	unsigned int numVerts = graph->get_num_verts();
	for (i = 0; i < numVerts; i++)
	{

		node_data *n = graph->get_vert(i);
		if (n->external) continue;

		if (n->nodeSym.size()) continue; //don't care about instruction in library call
		string blah = "";
		string *itext = &blah;
		
		if (a_coord_on_screen(n->vcoord.a, clientstate->leftcolumn, clientstate->rightcolumn, graph->m_scalefactors->HEDGESEP))
		{
			if (!show_all_always) {
				float nB = n->vcoord.b + n->vcoord.bMod*BMODMAG;

				if (zdist < 5 && clientstate->show_ins_text == INSTEXT_AUTO)
					itext = &n->ins->ins_text;
				else
					itext = &n->ins->mnemonic;
			}

			//todo: experiment with performance re:how much of this check to include
			
			DCOORD screenCoord = n->get_screen_pos(vertsdata, pd);

			if (screenCoord.x > clientstate->size.width || screenCoord.x < -100) continue;
			if (screenCoord.y > clientstate->size.height || screenCoord.y < -100) continue;

			stringstream ss;
			ss << std::hex << n->ins->address <<std::dec <<" ("<< n->index << "):" << *itext;
			al_draw_text(clientstate->standardFont, al_col_white, screenCoord.x + INS_X_OFF,
				clientstate->size.height - screenCoord.y + INS_Y_OFF, ALLEGRO_ALIGN_LEFT,
				//itext->c_str());
				ss.str().c_str());
			drawn++;
			
		}
	
	}
}

void draw_condition_ins_text(VISSTATE *clientstate, int zdist, PROJECTDATA *pd, GRAPH_DISPLAY_DATA *vertsdata)
{
	thread_graph_data *graph = (thread_graph_data *)clientstate->activeGraph;
	//iterate through nodes looking for ones that map to screen coords
	unsigned int i, drawn = 0;
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	bool show_all_always = (clientstate->show_ins_text == INSTEXT_ALL_ALWAYS);
	unsigned int numVerts = vertsdata->get_numVerts();
	GLfloat *vcol = vertsdata->readonly_col();
	for (i = 0; i < numVerts; i++)
	{
		node_data *n = graph->get_vert(i);
		if (!n->ins->conditional) continue;


		if (n->nodeSym.size()) continue; //don't care about instruction in library call
		string blah = "";
		string *itext = &blah;

		if (a_coord_on_screen(n->vcoord.a, clientstate->leftcolumn, clientstate->rightcolumn, graph->m_scalefactors->HEDGESEP))
		{
			if (!show_all_always) {
				float nB = n->vcoord.b + n->vcoord.bMod*BMODMAG;

				if (zdist < 5 && clientstate->show_ins_text == INSTEXT_AUTO)
					itext = &n->ins->ins_text;
				else
					itext = &n->ins->mnemonic;
			}

			//todo: experiment with performance re:how much of this check to include
			DCOORD screenCoord = n->get_screen_pos(vertsdata, pd);

			if (screenCoord.x > clientstate->size.width || screenCoord.x < -100) continue;
			if (screenCoord.y > clientstate->size.height || screenCoord.y < -100) continue;

			ALLEGRO_COLOR textcol;
			textcol.r = vcol[n->index*COLELEMS];
			textcol.g = vcol[(n->index*COLELEMS)+1];
			textcol.b = vcol[(n->index*COLELEMS)+2];
			textcol.a = 1;
			stringstream ss;
			ss << "0x" << std::hex << n->ins->address << ": " << std::dec << *itext;
			al_draw_text(clientstate->standardFont, textcol, screenCoord.x + INS_X_OFF,
				clientstate->size.height - screenCoord.y + 12, ALLEGRO_ALIGN_LEFT,
				//itext->c_str());
				ss.str().c_str());
			drawn++;
		}
	}
}
void draw_edge_heat_text(VISSTATE *clientstate, int zdist, PROJECTDATA *pd)
{
	thread_graph_data *graph = (thread_graph_data *)clientstate->activeGraph;
	//iterate through nodes looking for ones that map to screen coords
	unsigned int i, drawn = 0;
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	bool show_all_always = (clientstate->show_ins_text == INSTEXT_ALL_ALWAYS);
	GRAPH_DISPLAY_DATA *vertsdata = graph->get_mainverts();
	for (i = 0; i < graph->edgeList.size(); i++)
	{
		node_data *n = graph->get_vert(graph->edgeList[i].first);

		if (n->nodeSym.size()) continue; //don't care about instruction in library call
		string blah = "";
		string *itext = &blah;

		if (true || a_coord_on_screen(n->vcoord.a, clientstate->leftcolumn, clientstate->rightcolumn, graph->m_scalefactors->HEDGESEP))
		{
			if (graph->edgeDict[graph->edgeList[i]].weight < 2) continue;

			//todo: experiment with performance re:how much of this check to include
			DCOORD screenCoordA = n->get_screen_pos(vertsdata, pd);
			DCOORD screenCoordB = graph->get_vert(graph->edgeList[i].second)->get_screen_pos(vertsdata, pd);
			DCOORD screenCoord;
			midpoint(&screenCoordA, &screenCoordB, &screenCoord);

			if (screenCoord.x > clientstate->size.width || screenCoord.x < -100) continue;
			if (screenCoord.y > clientstate->size.height || screenCoord.y < -100) continue;

			stringstream ss;
			ss <<graph->edgeDict[graph->edgeList[i]].weight;
			al_draw_text(clientstate->standardFont, al_col_orange, screenCoord.x + INS_X_OFF,
				clientstate->size.height - screenCoord.y + INS_Y_OFF, ALLEGRO_ALIGN_LEFT,
				//itext->c_str());
				ss.str().c_str());
			drawn++;
		}
	}
}



GRAPH_DISPLAY_DATA * display_active_graph(VISSTATE *clientstate, thread_graph_data *graph)
{
	GRAPH_DISPLAY_DATA *vertsdata = graph->get_activeverts();
	GRAPH_DISPLAY_DATA *linedata = graph->get_activelines();

	if (graph->needVBOReload_active)
	{
		//todo - main ones probably already loaded?
		//void loadVBOs(GLuint *VBOs, GRAPH_DISPLAY_DATA *verts, GRAPH_DISPLAY_DATA *lines)
		//{
		//GLuint *VBOs = graph->activeVBOs;
		glGenBuffers(4, graph->activeVBOs);

		load_VBO(VBO_NODE_POS, graph->activeVBOs, graph->get_mainverts()->pos_size(), graph->get_mainverts()->readonly_pos());
		load_VBO(VBO_NODE_COL, graph->activeVBOs, graph->get_activeverts()->col_size(), graph->get_activeverts()->readonly_col());

		int posbufsize = graph->get_mainlines()->get_numVerts() * POSELEMS * sizeof(GLfloat);
		load_VBO(VBO_LINE_POS, graph->activeVBOs, posbufsize, graph->get_mainlines()->readonly_pos());

		int linebufsize = graph->get_activelines()->get_numVerts() * COLELEMS * sizeof(GLfloat);
		load_VBO(VBO_LINE_COL, graph->activeVBOs, linebufsize, graph->get_activelines()->readonly_col());

		//loadVBOs(graph->activeVBOs, vertsdata, linedata);
		graph->needVBOReload_active = false;
	}

	if (clientstate->modes.nodes)
		array_render_points(VBO_NODE_POS, VBO_NODE_COL, graph->activeVBOs, vertsdata->get_numVerts());

	if (clientstate->modes.edges)
		array_render_lines(VBO_LINE_POS, VBO_LINE_COL, graph->activeVBOs, linedata->get_numVerts());

	return vertsdata;
}

GRAPH_DISPLAY_DATA * display_static_graph(VISSTATE *clientstate, thread_graph_data *graph)
{
	GRAPH_DISPLAY_DATA *vertsdata = graph->get_mainverts();
	GRAPH_DISPLAY_DATA *linedata = graph->get_mainlines();
	if (graph->needVBOReload_main)
	{
		loadVBOs(graph->graphVBOs, vertsdata, linedata);
		graph->needVBOReload_main = false;
	}

	if (clientstate->modes.nodes)
		array_render_points(VBO_NODE_POS, VBO_NODE_COL, graph->graphVBOs, vertsdata->get_numVerts());

	if (clientstate->modes.edges)
		array_render_lines(VBO_LINE_POS, VBO_LINE_COL, graph->graphVBOs, linedata->get_numVerts());

	return vertsdata;
}

void display_graph(VISSTATE *clientstate, thread_graph_data *graph, PROJECTDATA *pd)
{
	GRAPH_DISPLAY_DATA *vertsdata;
	if (clientstate->modes.animation)
		vertsdata = display_active_graph(clientstate, graph);
	else
		vertsdata = display_static_graph(clientstate, graph);


	long graphSize = graph->m_scalefactors->radius;
	float zdiff = clientstate->zoomlevel - graphSize;
	float zmul = (clientstate->zoomlevel - graphSize) / 1000 - 1;
	
	if (zmul < 25)
		show_extern_labels(clientstate, pd, graph, vertsdata);

	if (clientstate->show_ins_text && zmul < 10 && graph->get_num_verts() > 2)
		draw_instruction_text(clientstate, zmul, pd, graph, vertsdata);


}

void display_graph_diff(VISSTATE *clientstate, diff_plotter *diffRenderer) {
	thread_graph_data *graph1 = diffRenderer->get_graph(1);
	thread_graph_data *diffgraph = diffRenderer->get_diff_graph();
	GRAPH_DISPLAY_DATA *vertsdata = graph1->get_mainverts();
	GRAPH_DISPLAY_DATA *linedata = graph1->get_mainlines();

	if (graph1->needVBOReload_main)
	{
		loadVBOs(graph1->graphVBOs, vertsdata, linedata);
		graph1->needVBOReload_main = false;
	}
	if (diffgraph->needVBOReload_main)
	{
		load_edge_VBOS(diffgraph->graphVBOs, diffgraph->get_mainlines());
		diffgraph->needVBOReload_main = false;
	}

	if (clientstate->modes.nodes)
		array_render_points(VBO_NODE_POS, VBO_NODE_COL, graph1->graphVBOs, vertsdata->get_numVerts());

	if (clientstate->modes.edges)
		array_render_lines(VBO_LINE_POS, VBO_LINE_COL, diffgraph->graphVBOs, linedata->get_numVerts());

	long graphSize = graph1->m_scalefactors->radius;
	float zdiff = clientstate->zoomlevel - graphSize;
	float zmul = (clientstate->zoomlevel - graphSize) / 1000 - 1;

	PROJECTDATA pd;
	bool pdgathered = false;
	if (zmul < 25)
	{
		gather_projection_data(&pd);
		pdgathered = true;
		show_extern_labels(clientstate, &pd, graph1, graph1->get_mainverts());
	}

	if (clientstate->show_ins_text && zmul < 10 && graph1->get_num_verts() > 2)
	{
		if (!pdgathered) 
			gather_projection_data(&pd);
		draw_instruction_text(clientstate, zmul, &pd, graph1, graph1->get_mainverts());
	}
}

void display_big_heatmap(VISSTATE *clientstate)
{
	thread_graph_data *graph = (thread_graph_data *)clientstate->activeGraph;
	if (!graph->heatmaplines) return;

	if (graph->needVBOReload_heatmap)
	{
		glGenBuffers(1, graph->heatmapEdgeVBO);
		glBindBuffer(GL_ARRAY_BUFFER, graph->heatmapEdgeVBO[0]);
		glBufferData(GL_ARRAY_BUFFER, graph->heatmaplines->col_size(), graph->heatmaplines->readonly_col(), GL_STATIC_DRAW);
		graph->needVBOReload_heatmap = false;
	}

	GRAPH_DISPLAY_DATA *vertsdata = graph->get_mainverts();
	GRAPH_DISPLAY_DATA *linedata = graph->get_mainlines();
	if (graph->needVBOReload_main)
	{
		loadVBOs(graph->graphVBOs, vertsdata, linedata);
		graph->needVBOReload_main = false;
	}

	if (clientstate->modes.nodes)
		array_render_points(VBO_NODE_POS, VBO_NODE_COL, graph->graphVBOs, vertsdata->get_numVerts());

	if (clientstate->modes.edges)
	{
		glBindBuffer(GL_ARRAY_BUFFER, graph->graphVBOs[VBO_LINE_POS]);
		glVertexPointer(3, GL_FLOAT, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, graph->heatmapEdgeVBO[0]);
		glColorPointer(4, GL_FLOAT, 0, 0);
		glDrawArrays(GL_LINES, 0, graph->heatmaplines->get_numVerts());
	}


	float zdiff = clientstate->zoomlevel - graph->zoomLevel;
	float zmul = (clientstate->zoomlevel - graph->zoomLevel) / 1000 - 1;

	PROJECTDATA pd;
	gather_projection_data(&pd);
	if (zmul < 25)
		show_extern_labels(clientstate, &pd, graph, vertsdata);

	if (clientstate->show_ins_text && zmul < 10 && graph->get_num_verts() > 2)
		draw_edge_heat_text(clientstate, zmul, &pd);
}

void display_big_conditional(VISSTATE *clientstate)
{
	thread_graph_data *graph = (thread_graph_data *)clientstate->activeGraph;
	if (!graph->conditionallines || !graph->conditionalverts) return;

	if (graph->needVBOReload_conditional)
	{
		glGenBuffers(2, graph->conditionalVBOs);
		glBindBuffer(GL_ARRAY_BUFFER, graph->conditionalVBOs[0]);
		glBufferData(GL_ARRAY_BUFFER, graph->conditionalverts->col_size(), graph->conditionalverts->readonly_col(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, graph->conditionalVBOs[1]);
		glBufferData(GL_ARRAY_BUFFER, graph->conditionallines->col_size(), graph->conditionallines->readonly_col(), GL_STATIC_DRAW);

		graph->needVBOReload_conditional = false;
	}

	if (graph->needVBOReload_main)
	{
		loadVBOs(graph->graphVBOs, graph->get_mainverts(), graph->get_mainlines());
		graph->needVBOReload_main = false;
	}

	if (clientstate->modes.nodes)
	{
		glBindBuffer(GL_ARRAY_BUFFER, graph->graphVBOs[VBO_NODE_POS]);
		glVertexPointer(3, GL_FLOAT, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, graph->conditionalVBOs[0]);
		glColorPointer(4, GL_FLOAT, 0, 0);
		glDrawArrays(GL_POINTS, 0, graph->conditionalverts->get_numVerts());
	}

	if (clientstate->modes.edges)
	{
		glBindBuffer(GL_ARRAY_BUFFER, graph->graphVBOs[VBO_LINE_POS]);
		glVertexPointer(3, GL_FLOAT, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, graph->conditionalVBOs[1]);
		glColorPointer(4, GL_FLOAT, 0, 0);
		glDrawArrays(GL_LINES, 0, graph->conditionallines->get_numVerts());
	}


	float zdiff = clientstate->zoomlevel - graph->zoomLevel;
	float zmul = (clientstate->zoomlevel - graph->zoomLevel) / 1000 - 1;

	PROJECTDATA pd;
	gather_projection_data(&pd);
	if (clientstate->show_ins_text && zmul < 10 && graph->get_num_verts() > 2)
	{
		
		draw_condition_ins_text(clientstate, zmul, &pd, graph->get_mainverts());
	}
}

void draw_anim_line(node_data *node, thread_graph_data *graph)
{
	if (!node) return;

	FCOORD center;
	center.x = 0;
	center.y = 0;
	center.z = 0;

	FCOORD nodeCoord;
	VCOORD *npos = &node->vcoord;
	float adjB = npos->b + float(npos->bMod * BMODMAG);
	sphereCoord(npos->a, adjB, &nodeCoord, graph->m_scalefactors, 0);
	drawRedLine(center, nodeCoord);
}