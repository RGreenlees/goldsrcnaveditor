//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include "SDL.h"
#include "SDL_opengl.h"
#ifdef __APPLE__
#	include <OpenGL/glu.h>
#else
#	include <GL/glu.h>
#endif
#include "imgui.h"
#include "OffMeshConnectionTool.h"
#include "InputGeom.h"
#include "Sample.h"
#include "Recast.h"
#include "RecastDebugDraw.h"
#include "DetourDebugDraw.h"

#include "NavProfiles.h"

#ifdef WIN32
#	define snprintf _snprintf
#endif

OffMeshConnectionTool::OffMeshConnectionTool() :
	m_sample(0),
	m_hitPosSet(0),
	m_bidir(true),
	m_oldFlags(0)
{
}

OffMeshConnectionTool::~OffMeshConnectionTool()
{
	if (m_sample)
	{
		m_sample->setNavMeshDrawFlags(m_oldFlags);
	}
}

void OffMeshConnectionTool::init(Sample* sample)
{
	if (m_sample != sample)
	{
		m_sample = sample;
		m_oldFlags = m_sample->getNavMeshDrawFlags();
		m_sample->setNavMeshDrawFlags(m_oldFlags & ~DU_DRAWNAVMESH_OFFMESHCONS);
	}
}

void OffMeshConnectionTool::reset()
{
	m_hitPosSet = false;
}

void OffMeshConnectionTool::handleMenu()
{
	if (imguiCheck("One Way", !m_bidir))
		m_bidir = false;
	if (imguiCheck("Bidirectional", m_bidir))
		m_bidir = true;

	imguiSeparator();

	vector<NavOffMeshConnectionDefinition> AllConnectionTypes = GetAllConnectionDefinitions();

	int thisIndex = 0;

	for (auto it = AllConnectionTypes.begin(); it != AllConnectionTypes.end(); it++)
	{
		if (imguiCheck(it->ConnName.c_str(), m_connIndex == thisIndex))
		{
			m_connIndex = thisIndex;
		}
		thisIndex++;
	}
}

void OffMeshConnectionTool::handleClick(const float* /*s*/, const float* p, bool shift)
{
	if (!m_sample) return;
	InputGeom* geom = m_sample->getInputGeom();
	if (!geom) return;

	if (shift)
	{
		m_sample->removeOffMeshConnection(p);
	}
	else
	{
		// Create	
		if (!m_hitPosSet)
		{
			rcVcopy(m_hitPos, p);
			m_hitPosSet = true;
		}
		else
		{
			NavOffMeshConnectionDefinition* SelectedConnectionType = GetConnectionAtIndex(m_connIndex);

			if (!SelectedConnectionType)
			{
				m_connIndex = 0;
				return;
			}

			NavAreaDefinition* ConnArea = GetAreaAtIndex(SelectedConnectionType->AreaIndex);
			NavFlagDefinition* ConnFlag = GetFlagAtIndex(SelectedConnectionType->FlagIndex);

			if (!ConnArea || !ConnFlag) { return; }

			m_sample->addOffMeshConnection(m_hitPos, p, m_sample->getAgentRadius(), ConnArea->AreaId, ConnFlag->FlagId, m_bidir ? 1 : 0);
			m_hitPosSet = false;
		}
	}
	
}

void OffMeshConnectionTool::handleToggle()
{
}

void OffMeshConnectionTool::handleStep()
{
}

void OffMeshConnectionTool::handleUpdate(const float /*dt*/)
{
}

void OffMeshConnectionTool::handleRender()
{
	duDebugDraw& dd = m_sample->getDebugDraw();
	const float s = m_sample->getAgentRadius();
	
	if (m_hitPosSet)
		duDebugDrawCross(&dd, m_hitPos[0],m_hitPos[1]+0.1f,m_hitPos[2], s, duRGBA(0,0,0,128), 2.0f);

	if (m_sample)
	{
		m_sample->drawOffMeshConnections(&dd);
	}
}

void OffMeshConnectionTool::handleRenderOverlay(double* proj, double* model, int* view)
{
	GLdouble x, y, z;
	
	// Draw start and end point labels
	if (m_hitPosSet && gluProject((GLdouble)m_hitPos[0], (GLdouble)m_hitPos[1], (GLdouble)m_hitPos[2],
								model, proj, view, &x, &y, &z))
	{
		imguiDrawText((int)x, (int)(y-25), IMGUI_ALIGN_CENTER, "Start", imguiRGBA(0,0,0,220));
	}
	
	// Tool help
	const int h = view[3];
	if (!m_hitPosSet)
	{
		imguiDrawText(280, h-40, IMGUI_ALIGN_LEFT, "LMB: Create new connection.  SHIFT+LMB: Delete existing connection, click close to start or end point.", imguiRGBA(255,255,255,192));	
	}
	else
	{
		imguiDrawText(280, h-40, IMGUI_ALIGN_LEFT, "LMB: Set connection end point and finish.", imguiRGBA(255,255,255,192));	
	}
}
