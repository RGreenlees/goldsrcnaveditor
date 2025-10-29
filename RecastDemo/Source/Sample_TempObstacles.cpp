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
#include <algorithm>
#include <float.h>
#include <new>
#include "SDL.h"
#include "SDL_opengl.h"
#ifdef __APPLE__
#	include <OpenGL/glu.h>
#else
#	include <GL/glu.h>
#endif
#include "imgui.h"
#include "InputGeom.h"
#include "Sample.h"
#include "Sample_TempObstacles.h"
#include "Recast.h"
#include "RecastDebugDraw.h"
#include "DetourAssert.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourDebugDraw.h"
#include "DetourCommon.h"
#include "DetourTileCache.h"
#include "NavMeshTesterTool.h"
#include "OffMeshConnectionTool.h"
#include "ConvexVolumeTool.h"
#include "CrowdTool.h"
#include "NavHintTool.h"
#include "RecastAlloc.h"
#include "RecastAssert.h"
#include "fastlz.h"
#include "Filelist.h"

#include "NavProfiles.h"
#include "MeshEditorTool.h"

#ifdef WIN32
#	define snprintf _snprintf
#endif


// This value specifies how many layers (or "floors") each navmesh tile is expected to have.
static const int EXPECTED_LAYERS_PER_TILE = 4;


static bool isectSegAABB(const float* sp, const float* sq,
						 const float* amin, const float* amax,
						 float& tmin, float& tmax)
{
	static const float EPS = 1e-6f;

	float d[3];
	rcVsub(d, sq, sp);
	tmin = 0;  // set to -FLT_MAX to get first hit on line
	tmax = FLT_MAX;		// set to max distance ray can travel (for segment)

	// For all three slabs
	for (int i = 0; i < 3; i++)
	{
		if (fabsf(d[i]) < EPS)
		{
			// Ray is parallel to slab. No hit if origin not within slab
			if (sp[i] < amin[i] || sp[i] > amax[i])
				return false;
		}
		else
		{
			// Compute intersection t value of ray with near and far plane of slab
			const float ood = 1.0f / d[i];
			float t1 = (amin[i] - sp[i]) * ood;
			float t2 = (amax[i] - sp[i]) * ood;
			// Make t1 be intersection with near plane, t2 with far plane
			if (t1 > t2) rcSwap(t1, t2);
			// Compute the intersection of slab intersections intervals
			if (t1 > tmin) tmin = t1;
			if (t2 < tmax) tmax = t2;
			// Exit with no collision as soon as slab intersection becomes empty
			if (tmin > tmax) return false;
		}
	}

	return true;
}

static int calcLayerBufferSize(const int gridWidth, const int gridHeight)
{
	const int headerSize = dtAlign4(sizeof(dtTileCacheLayerHeader));
	const int gridSize = gridWidth * gridHeight;
	return headerSize + gridSize*4;
}

struct FastLZCompressor : public dtTileCacheCompressor
{
	virtual ~FastLZCompressor();

	virtual int maxCompressedSize(const int bufferSize)
	{
		return (int)(bufferSize* 1.05f);
	}

	virtual dtStatus compress(const unsigned char* buffer, const int bufferSize,
							  unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize)
	{
		*compressedSize = fastlz_compress((const void *const)buffer, bufferSize, compressed);
		return DT_SUCCESS;
	}

	virtual dtStatus decompress(const unsigned char* compressed, const int compressedSize,
								unsigned char* buffer, const int maxBufferSize, int* bufferSize)
	{
		*bufferSize = fastlz_decompress(compressed, compressedSize, buffer, maxBufferSize);
		return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
	}
};

FastLZCompressor::~FastLZCompressor()
{
	// Defined out of line to fix the weak v-tables warning
}

struct LinearAllocator : public dtTileCacheAlloc
{
	unsigned char* buffer;
	size_t capacity;
	size_t top;
	size_t high;

	LinearAllocator(const size_t cap) : buffer(0), capacity(0), top(0), high(0)
	{
		resize(cap);
	}

	virtual ~LinearAllocator();

	void resize(const size_t cap)
	{
		if (buffer) dtFree(buffer);
		buffer = (unsigned char*)dtAlloc(cap, DT_ALLOC_PERM);
		capacity = cap;
	}

	virtual void reset()
	{
		high = dtMax(high, top);
		top = 0;
	}

	virtual void* alloc(const size_t size)
	{
		if (!buffer)
			return 0;
		if (top+size > capacity)
			return 0;
		unsigned char* mem = &buffer[top];
		top += size;
		return mem;
	}

	virtual void free(void* /*ptr*/)
	{
		// Empty
	}
};

LinearAllocator::~LinearAllocator()
{
	// Defined out of line to fix the weak v-tables warning
	dtFree(buffer);
}

struct MeshProcess : public dtTileCacheMeshProcess
{
	InputGeom* m_geom;

	inline MeshProcess() : m_geom(0)
	{
	}

	virtual ~MeshProcess();

	inline void init(InputGeom* geom)
	{
		m_geom = geom;
	}

	virtual void process(struct dtNavMeshCreateParams* params,
						 unsigned char* polyAreas, unsigned int* polyFlags)
	{
		// Update poly flags from areas.
		for (int i = 0; i < params->polyCount; ++i)
		{
			unsigned char thisArea = polyAreas[i];

			NavAreaDefinition* Area = GetAreaAtIndex(polyAreas[i]);

			if (Area)
			{
				polyAreas[i] = Area->AreaId;

				NavFlagDefinition* Flag = GetFlagAtIndex(Area->FlagIndex);

				if (Flag)
				{
					polyFlags[i] = Flag->FlagId;
				}
			}
		}

		// Pass in off-mesh connections.
		if (m_geom)
		{
			params->offMeshConVerts = m_geom->getOffMeshConnectionVerts();
			params->offMeshConRad = m_geom->getOffMeshConnectionRads();
			params->offMeshConDir = m_geom->getOffMeshConnectionDirs();
			params->offMeshConAreas = m_geom->getOffMeshConnectionAreas();
			params->offMeshConFlags = m_geom->getOffMeshConnectionFlags();
			params->offMeshConUserID = m_geom->getOffMeshConnectionId();
			params->offMeshConCount = m_geom->getOffMeshConnectionCount();
		}
	}
};

MeshProcess::~MeshProcess()
{
	// Defined out of line to fix the weak v-tables warning
}

static const int MAX_LAYERS = 32;

struct TileCacheData
{
	unsigned char* data;
	int dataSize;
};

struct RasterizationContext
{
	RasterizationContext() :
		solid(0),
		triareas(0),
		lset(0),
		chf(0),
		ntiles(0)
	{
		memset(tiles, 0, sizeof(TileCacheData)*MAX_LAYERS);
	}

	~RasterizationContext()
	{
		rcFreeHeightField(solid);
		delete [] triareas;
		rcFreeHeightfieldLayerSet(lset);
		rcFreeCompactHeightfield(chf);
		for (int i = 0; i < MAX_LAYERS; ++i)
		{
			dtFree(tiles[i].data);
			tiles[i].data = 0;
		}
	}

	rcHeightfield* solid;
	unsigned char* triareas;
	rcHeightfieldLayerSet* lset;
	rcCompactHeightfield* chf;
	TileCacheData tiles[MAX_LAYERS];
	int ntiles;
};

int Sample_TempObstacles::rasterizeTileLayers(
								const unsigned int NavMeshIndex,
								const int tx, const int ty,
								const rcConfig& cfg,
								TileCacheData* tiles,
								const int maxTiles)
{
	if (!m_geom || !m_geom->getMesh() || !m_geom->getChunkyMesh())
	{
		m_ctx->log(RC_LOG_ERROR, "buildTile: Input mesh is not specified.");
		return 0;
	}

	FastLZCompressor comp;
	RasterizationContext rc;

	const float* verts = m_geom->getMesh()->getVerts();
	const int nverts = m_geom->getMesh()->getVertCount();
	const rcChunkyTriMesh* chunkyMesh = m_geom->getChunkyMesh();

	// Tile bounds.
	const float tcs = cfg.tileSize * cfg.cs;

	rcConfig tcfg;
	memcpy(&tcfg, &cfg, sizeof(tcfg));

	tcfg.bmin[0] = cfg.bmin[0] + tx*tcs;
	tcfg.bmin[1] = cfg.bmin[1];
	tcfg.bmin[2] = cfg.bmin[2] + ty*tcs;
	tcfg.bmax[0] = cfg.bmin[0] + (tx+1)*tcs;
	tcfg.bmax[1] = cfg.bmax[1];
	tcfg.bmax[2] = cfg.bmin[2] + (ty+1)*tcs;
	tcfg.bmin[0] -= tcfg.borderSize*tcfg.cs;
	tcfg.bmin[2] -= tcfg.borderSize*tcfg.cs;
	tcfg.bmax[0] += tcfg.borderSize*tcfg.cs;
	tcfg.bmax[2] += tcfg.borderSize*tcfg.cs;

	// Allocate voxel heightfield where we rasterize our input data to.
	rc.solid = rcAllocHeightfield();
	if (!rc.solid)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
		return 0;
	}
	if (!rcCreateHeightfield(m_ctx, *rc.solid, tcfg.width, tcfg.height, tcfg.bmin, tcfg.bmax, tcfg.cs, tcfg.ch))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
		return 0;
	}

	// Allocate array that can hold triangle flags.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	rc.triareas = new unsigned char[chunkyMesh->maxTrisPerChunk];
	if (!rc.triareas)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'm_triareas' (%d).", chunkyMesh->maxTrisPerChunk);
		return 0;
	}

	float tbmin[2], tbmax[2];
	tbmin[0] = tcfg.bmin[0];
	tbmin[1] = tcfg.bmin[2];
	tbmax[0] = tcfg.bmax[0];
	tbmax[1] = tcfg.bmax[2];
	int cid[512];// TODO: Make grow when returning too many items.
	const int ncid = rcGetChunksOverlappingRect(chunkyMesh, tbmin, tbmax, cid, 512);
	if (!ncid)
	{
		return 0; // empty
	}

	for (int i = 0; i < ncid; ++i)
	{
		const rcChunkyTriMeshNode& node = chunkyMesh->nodes[cid[i]];
		const int* tris = &chunkyMesh->tris[node.i*3];
		const int* surfTypes = &chunkyMesh->surfTypes[node.i];
		const int ntris = node.n;

		memset(rc.triareas, 0, ntris*sizeof(unsigned char));

		rcMarkWalkableTriangles(m_ctx, tcfg.walkableSlopeAngle,	verts, nverts, tris, ntris, rc.triareas, surfTypes);

		if (!rcRasterizeTriangles(m_ctx, verts, nverts, tris, rc.triareas, ntris, *rc.solid, tcfg.walkableClimb))
			return 0;
	}

	// Once all geometry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	if (m_filterLowHangingObstacles)
		rcFilterLowHangingWalkableObstacles(m_ctx, tcfg.walkableClimb, *rc.solid);
	if (m_filterLedgeSpans)
		rcFilterLedgeSpans(m_ctx, tcfg.walkableHeight, tcfg.walkableClimb, *rc.solid);
	if (m_filterWalkableLowHeightSpans)
		rcFilterWalkableLowHeightSpans(m_ctx, tcfg.walkableHeight, cfg.crouchHeight, *rc.solid);


	rc.chf = rcAllocCompactHeightfield();
	if (!rc.chf)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
		return 0;
	}
	if (!rcBuildCompactHeightfield(m_ctx, 13, tcfg.walkableClimb, *rc.solid, *rc.chf))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
		return 0;
	}

	// Erode the walkable area by agent radius.
	if (!rcErodeWalkableArea(m_ctx, tcfg.walkableRadius, *rc.chf))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
		return 0;
	}

	// (Optional) Mark areas.
	const ConvexVolume* vols = m_geom->getConvexVolumes();
	for (int i  = 0; i < m_geom->getConvexVolumeCount(); ++i)
	{
		if (vols[i].NavMeshIndex != NavMeshIndex) { continue; }

		rcMarkConvexPolyArea(m_ctx, vols[i].verts, vols[i].nverts,
							 vols[i].hmin, vols[i].hmax,
							 (unsigned char)vols[i].area, *rc.chf);
	}

	rc.lset = rcAllocHeightfieldLayerSet();
	if (!rc.lset)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'lset'.");
		return 0;
	}
	if (!rcBuildHeightfieldLayers(m_ctx, *rc.chf, tcfg.borderSize, cfg.crouchHeight, *rc.lset))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build heighfield layers.");
		return 0;
	}

	rc.ntiles = 0;
	for (int i = 0; i < rcMin(rc.lset->nlayers, MAX_LAYERS); ++i)
	{
		TileCacheData* tile = &rc.tiles[rc.ntiles++];
		const rcHeightfieldLayer* layer = &rc.lset->layers[i];

		// Store header
		dtTileCacheLayerHeader header;
		header.magic = DT_TILECACHE_MAGIC;
		header.version = DT_TILECACHE_VERSION;

		// Tile layer location in the navmesh.
		header.tx = tx;
		header.ty = ty;
		header.tlayer = i;
		dtVcopy(header.bmin, layer->bmin);
		dtVcopy(header.bmax, layer->bmax);

		// Tile info.
		header.width = (unsigned char)layer->width;
		header.height = (unsigned char)layer->height;
		header.minx = (unsigned char)layer->minx;
		header.maxx = (unsigned char)layer->maxx;
		header.miny = (unsigned char)layer->miny;
		header.maxy = (unsigned char)layer->maxy;
		header.hmin = (unsigned short)layer->hmin;
		header.hmax = (unsigned short)layer->hmax;

		dtStatus status = dtBuildTileCacheLayer(&comp, &header, layer->heights, layer->areas, layer->cons,
												&tile->data, &tile->dataSize);
		if (dtStatusFailed(status))
		{
			return 0;
		}
	}

	// Transfer ownsership of tile data from build context to the caller.
	int n = 0;
	for (int i = 0; i < rcMin(rc.ntiles, maxTiles); ++i)
	{
		tiles[n++] = rc.tiles[i];
		rc.tiles[i].data = 0;
		rc.tiles[i].dataSize = 0;
	}

	return n;
}


void drawTiles(duDebugDraw* dd, dtTileCache* tc)
{
	unsigned int fcol[6];
	float bmin[3], bmax[3];

	for (int i = 0; i < tc->getTileCount(); ++i)
	{
		const dtCompressedTile* tile = tc->getTile(i);
		if (!tile->header) continue;

		tc->calcTightTileBounds(tile->header, bmin, bmax);

		const unsigned int col = duIntToCol(i,64);
		duCalcBoxColors(fcol, col, col);
		duDebugDrawBox(dd, bmin[0],bmin[1],bmin[2], bmax[0],bmax[1],bmax[2], fcol);
	}

	for (int i = 0; i < tc->getTileCount(); ++i)
	{
		const dtCompressedTile* tile = tc->getTile(i);
		if (!tile->header) continue;

		tc->calcTightTileBounds(tile->header, bmin, bmax);

		const unsigned int col = duIntToCol(i,255);
		const float pad = tc->getParams()->cs * 0.1f;
		duDebugDrawBoxWire(dd, bmin[0]-pad,bmin[1]-pad,bmin[2]-pad,
						   bmax[0]+pad,bmax[1]+pad,bmax[2]+pad, col, 2.0f);
	}

}

enum DrawDetailType
{
	DRAWDETAIL_AREAS,
	DRAWDETAIL_REGIONS,
	DRAWDETAIL_CONTOURS,
	DRAWDETAIL_MESH
};

void drawDetail(duDebugDraw* dd, dtTileCache* tc, const int tx, const int ty, int type)
{
	struct TileCacheBuildContext
	{
		inline TileCacheBuildContext(struct dtTileCacheAlloc* a) : layer(0), lcset(0), lmesh(0), alloc(a) {}
		inline ~TileCacheBuildContext() { purge(); }
		void purge()
		{
			dtFreeTileCacheLayer(alloc, layer);
			layer = 0;
			dtFreeTileCacheContourSet(alloc, lcset);
			lcset = 0;
			dtFreeTileCachePolyMesh(alloc, lmesh);
			lmesh = 0;
		}
		struct dtTileCacheLayer* layer;
		struct dtTileCacheContourSet* lcset;
		struct dtTileCachePolyMesh* lmesh;
		struct dtTileCacheAlloc* alloc;
	};

	dtCompressedTileRef tiles[MAX_LAYERS];
	const int ntiles = tc->getTilesAt(tx,ty,tiles,MAX_LAYERS);

	dtTileCacheAlloc* talloc = tc->getAlloc();
	dtTileCacheCompressor* tcomp = tc->getCompressor();
	const dtTileCacheParams* params = tc->getParams();

	for (int i = 0; i < ntiles; ++i)
	{
		const dtCompressedTile* tile = tc->getTileByRef(tiles[i]);

		talloc->reset();

		TileCacheBuildContext bc(talloc);
		const int walkableClimbVx = (int)(params->walkableClimb / params->ch);
		dtStatus status;

		// Decompress tile layer data.
		status = dtDecompressTileCacheLayer(talloc, tcomp, tile->data, tile->dataSize, &bc.layer);
		if (dtStatusFailed(status))
			return;
		if (type == DRAWDETAIL_AREAS)
		{
			duDebugDrawTileCacheLayerAreas(dd, *bc.layer, params->cs, params->ch);
			continue;
		}

		// Build navmesh
		status = dtBuildTileCacheRegions(talloc, *bc.layer, walkableClimbVx);
		if (dtStatusFailed(status))
			return;
		if (type == DRAWDETAIL_REGIONS)
		{
			duDebugDrawTileCacheLayerRegions(dd, *bc.layer, params->cs, params->ch);
			continue;
		}

		bc.lcset = dtAllocTileCacheContourSet(talloc);
		if (!bc.lcset)
			return;
		status = dtBuildTileCacheContours(talloc, *bc.layer, walkableClimbVx,
										  params->maxSimplificationError, *bc.lcset);
		if (dtStatusFailed(status))
			return;
		if (type == DRAWDETAIL_CONTOURS)
		{
			duDebugDrawTileCacheContours(dd, *bc.lcset, tile->header->bmin, params->cs, params->ch);
			continue;
		}

		bc.lmesh = dtAllocTileCachePolyMesh(talloc);
		if (!bc.lmesh)
			return;
		status = dtBuildTileCachePolyMesh(talloc, *bc.lcset, *bc.lmesh);
		if (dtStatusFailed(status))
			return;

		if (type == DRAWDETAIL_MESH)
		{
			duDebugDrawTileCachePolyMesh(dd, *bc.lmesh, tile->header->bmin, params->cs, params->ch);
			continue;
		}

	}
}


void drawDetailOverlay(const dtTileCache* tc, const int tx, const int ty, double* proj, double* model, int* view)
{
	dtCompressedTileRef tiles[MAX_LAYERS];
	const int ntiles = tc->getTilesAt(tx,ty,tiles,MAX_LAYERS);
	if (!ntiles)
		return;

	const int rawSize = calcLayerBufferSize(tc->getParams()->width, tc->getParams()->height);

	char text[128];

	for (int i = 0; i < ntiles; ++i)
	{
		const dtCompressedTile* tile = tc->getTileByRef(tiles[i]);

		float pos[3];
		pos[0] = (tile->header->bmin[0]+tile->header->bmax[0])/2.0f;
		pos[1] = tile->header->bmin[1];
		pos[2] = (tile->header->bmin[2]+tile->header->bmax[2])/2.0f;

		GLdouble x, y, z;
		if (gluProject((GLdouble)pos[0], (GLdouble)pos[1], (GLdouble)pos[2],
					   model, proj, view, &x, &y, &z))
		{
			snprintf(text,128,"(%d,%d)/%d", tile->header->tx,tile->header->ty,tile->header->tlayer);
			imguiDrawText((int)x, (int)y-25, IMGUI_ALIGN_CENTER, text, imguiRGBA(0,0,0,220));
			snprintf(text,128,"Compressed: %.1f kB", tile->dataSize/1024.0f);
			imguiDrawText((int)x, (int)y-45, IMGUI_ALIGN_CENTER, text, imguiRGBA(0,0,0,128));
			snprintf(text,128,"Raw:%.1fkB", rawSize/1024.0f);
			imguiDrawText((int)x, (int)y-65, IMGUI_ALIGN_CENTER, text, imguiRGBA(0,0,0,128));
		}
	}
}

dtObstacleRef hitTestObstacle(const dtTileCache* tc, const float* sp, const float* sq)
{
	float tmin = FLT_MAX;
	const dtTileCacheObstacle* obmin = 0;
	for (int i = 0; i < tc->getObstacleCount(); ++i)
	{
		const dtTileCacheObstacle* ob = tc->getObstacle(i);
		if (ob->state == DT_OBSTACLE_EMPTY)
			continue;

		float bmin[3], bmax[3], t0,t1;
		tc->getObstacleBounds(ob, bmin,bmax);

		if (isectSegAABB(sp,sq, bmin,bmax, t0,t1))
		{
			if (t0 < tmin)
			{
				tmin = t0;
				obmin = ob;
			}
		}
	}
	return tc->getObstacleRef(obmin);
}

void drawObstacles(duDebugDraw* dd, const dtTileCache* tc)
{
	// Draw obstacles
	for (int i = 0; i < tc->getObstacleCount(); ++i)
	{
		const dtTileCacheObstacle* ob = tc->getObstacle(i);
		if (ob->state == DT_OBSTACLE_EMPTY) continue;
		float bmin[3], bmax[3];
		tc->getObstacleBounds(ob, bmin,bmax);

		unsigned int col = 0;
		if (ob->state == DT_OBSTACLE_PROCESSING)
			col = duRGBA(255,255,0,128);
		else if (ob->state == DT_OBSTACLE_PROCESSED)
			col = duRGBA(255,192,0,192);
		else if (ob->state == DT_OBSTACLE_REMOVING)
			col = duRGBA(220,0,0,128);

		duDebugDrawCylinder(dd, bmin[0],bmin[1],bmin[2], bmax[0],bmax[1],bmax[2], col);
		duDebugDrawCylinderWire(dd, bmin[0],bmin[1],bmin[2], bmax[0],bmax[1],bmax[2], duDarkenCol(col), 2);
	}
}

class TempObstacleHilightTool : public SampleTool
{
	Sample_TempObstacles* m_sample;
	float m_hitPos[3];
	bool m_hitPosSet;
	int m_drawType;

public:

	TempObstacleHilightTool() :
		m_sample(0),
		m_hitPosSet(false),
		m_drawType(DRAWDETAIL_AREAS)
	{
		m_hitPos[0] = m_hitPos[1] = m_hitPos[2] = 0;
	}

	virtual ~TempObstacleHilightTool();

	virtual int type() { return TOOL_TILE_HIGHLIGHT; }

	virtual void init(Sample* sample)
	{
		m_sample = (Sample_TempObstacles*)sample;
	}

	virtual void reset() {}

	virtual void handleMenu()
	{
		imguiLabel("Highlight Tile Cache");
		imguiValue("Click LMB to highlight a tile.");
		imguiSeparator();
		if (imguiCheck("Draw Areas", m_drawType == DRAWDETAIL_AREAS))
			m_drawType = DRAWDETAIL_AREAS;
		if (imguiCheck("Draw Regions", m_drawType == DRAWDETAIL_REGIONS))
			m_drawType = DRAWDETAIL_REGIONS;
		if (imguiCheck("Draw Contours", m_drawType == DRAWDETAIL_CONTOURS))
			m_drawType = DRAWDETAIL_CONTOURS;
		if (imguiCheck("Draw Mesh", m_drawType == DRAWDETAIL_MESH))
			m_drawType = DRAWDETAIL_MESH;
	}

	virtual void handleClick(const float* /*s*/, const float* p, bool /*shift*/)
	{
		m_hitPosSet = true;
		rcVcopy(m_hitPos,p);
	}

	virtual void handleToggle() {}

	virtual void handleStep() {}

	virtual void handleUpdate(const float /*dt*/) {}

	virtual void handleRender()
	{
		if (m_hitPosSet && m_sample)
		{
			const float s = m_sample->getAgentRadius();
			glColor4ub(0,0,0,128);
			glLineWidth(2.0f);
			glBegin(GL_LINES);
			glVertex3f(m_hitPos[0]-s,m_hitPos[1]+0.1f,m_hitPos[2]);
			glVertex3f(m_hitPos[0]+s,m_hitPos[1]+0.1f,m_hitPos[2]);
			glVertex3f(m_hitPos[0],m_hitPos[1]-s+0.1f,m_hitPos[2]);
			glVertex3f(m_hitPos[0],m_hitPos[1]+s+0.1f,m_hitPos[2]);
			glVertex3f(m_hitPos[0],m_hitPos[1]+0.1f,m_hitPos[2]-s);
			glVertex3f(m_hitPos[0],m_hitPos[1]+0.1f,m_hitPos[2]+s);
			glEnd();
			glLineWidth(1.0f);

			int tx=0, ty=0;
			m_sample->getTilePos(m_hitPos, tx, ty);
			m_sample->renderCachedTile(tx,ty,m_drawType);
		}
	}

	virtual void handleRenderOverlay(double* proj, double* model, int* view)
	{
		if (m_hitPosSet)
		{
			if (m_sample)
			{
				int tx=0, ty=0;
				m_sample->getTilePos(m_hitPos, tx, ty);
				m_sample->renderCachedTileOverlay(tx,ty,proj,model,view);
			}
		}
	}
};

TempObstacleHilightTool::~TempObstacleHilightTool()
{
	// Defined out of line to fix the weak v-tables warning
}

class TempObstacleCreateTool : public SampleTool
{
	Sample_TempObstacles* m_sample;
	unsigned char m_Area = 0;

public:

	TempObstacleCreateTool() : m_sample(0)
	{
	}

	virtual ~TempObstacleCreateTool();

	virtual int type() { return TOOL_TEMP_OBSTACLE; }

	virtual void init(Sample* sample)
	{
		m_sample = (Sample_TempObstacles*)sample;
	}

	virtual void reset() {}

	virtual void handleMenu()
	{
		imguiLabel("Create Temp Obstacles");

		if (imguiCheck("Null", m_Area == 0))
		{
			m_Area = 0;
		}

		vector<NavAreaDefinition> AllAreas = GetAllNavAreaDefinitions();

		for (auto it = AllAreas.begin(); it != AllAreas.end(); it++)
		{
			if (imguiCheck(it->AreaName.c_str(), m_Area == it->AreaId))
			{
				m_Area = it->AreaId;
			}
		}

		if (imguiButton("Remove All"))
			m_sample->clearAllTempObstacles();

		imguiSeparator();

		imguiValue("Click LMB to create an obstacle.");
		imguiValue("Shift+LMB to remove an obstacle.");
	}

	virtual void handleClick(const float* s, const float* p, bool shift)
	{
		if (m_sample)
		{
			if (shift)
				m_sample->removeTempObstacle(s,p);
			else
				m_sample->addTempObstacle(p, m_Area);
		}
	}

	virtual void handleToggle() {}
	virtual void handleStep() {}
	virtual void handleUpdate(const float /*dt*/) {}
	virtual void handleRender() {}
	virtual void handleRenderOverlay(double* /*proj*/, double* /*model*/, int* /*view*/) { }
};

TempObstacleCreateTool::~TempObstacleCreateTool()
{
	// Defined out of line to fix the weak v-tables warning
}

Sample_TempObstacles::Sample_TempObstacles() :
	m_keepInterResults(false),
	m_tileCache(0),
	m_cacheBuildTimeMs(0),
	m_cacheCompressedSize(0),
	m_cacheRawSize(0),
	m_cacheLayerCount(0),
	m_cacheBuildMemUsage(0),
	m_drawMode(DRAWMODE_NAVMESH),
	m_maxTiles(0),
	m_maxPolysPerTile(0),
	m_tileSize(48)
{
	resetCommonSettings();

	m_talloc = new LinearAllocator(32000);
	m_tcomp = new FastLZCompressor;
	m_tmproc = new MeshProcess;

	setTool(new TempObstacleCreateTool);
}

Sample_TempObstacles::~Sample_TempObstacles()
{
	dtFreeNavMesh(m_navMesh);
	m_navMesh = 0;
	dtFreeTileCache(m_tileCache);
}

void Sample_TempObstacles::handleSettings()
{
	Sample::handleCommonSettings();

	if (imguiCheck("Keep Intermediate Results", m_keepInterResults))
		m_keepInterResults = !m_keepInterResults;

	imguiLabel("Tiling");
	imguiSlider("TileSize", &m_tileSize, 16.0f, 128.0f, 8.0f);

	int gridSize = 1;
	if (m_geom)
	{
		const float* bmin = m_geom->getNavMeshBoundsMin();
		const float* bmax = m_geom->getNavMeshBoundsMax();
		char text[64];
		int gw = 0, gh = 0;
		rcCalcGridSize(bmin, bmax, m_cellSize, &gw, &gh);
		const int ts = (int)m_tileSize;
		const int tw = (gw + ts-1) / ts;
		const int th = (gh + ts-1) / ts;
		snprintf(text, 64, "Tiles  %d x %d", tw, th);
		imguiValue(text);

		// Max tiles and max polys affect how the tile IDs are caculated.
		// There are 22 bits available for identifying a tile and a polygon.
		int tileBits = rcMin((int)dtIlog2(dtNextPow2(tw*th*EXPECTED_LAYERS_PER_TILE)), 14);
		if (tileBits > 14) tileBits = 14;
		int polyBits = 22 - tileBits;
		m_maxTiles = 1 << tileBits;
		m_maxPolysPerTile = 1 << polyBits;
		snprintf(text, 64, "Max Tiles  %d", m_maxTiles);
		imguiValue(text);
		snprintf(text, 64, "Max Polys  %d", m_maxPolysPerTile);
		imguiValue(text);
		gridSize = tw*th;
	}
	else
	{
		m_maxTiles = 0;
		m_maxPolysPerTile = 0;
	}

	imguiSeparator();

	imguiLabel("Tile Cache");
	char msg[64];

	const float compressionRatio = (float)m_cacheCompressedSize / (float)(m_cacheRawSize+1);

	snprintf(msg, 64, "Layers  %d", m_cacheLayerCount);
	imguiValue(msg);
	snprintf(msg, 64, "Layers (per tile)  %.1f", (float)m_cacheLayerCount/(float)gridSize);
	imguiValue(msg);

	snprintf(msg, 64, "Memory  %.1f kB / %.1f kB (%.1f%%)", m_cacheCompressedSize/1024.0f, m_cacheRawSize/1024.0f, compressionRatio*100.0f);
	imguiValue(msg);
	snprintf(msg, 64, "Navmesh Build Time  %.1f ms", m_cacheBuildTimeMs);
	imguiValue(msg);
	snprintf(msg, 64, "Build Peak Mem Usage  %.1f kB", m_cacheBuildMemUsage/1024.0f);
	imguiValue(msg);

	imguiSeparator();

	imguiIndent();
	imguiIndent();

	if (imguiButton("Save"))
	{
		unsigned char trimChars[3] = "/\\";
		string sanitisedPath = trim(GetCurrentGameProfile()->GameDirectory, trimChars, 2)
							 + "/"
							 + trim(GetCurrentGameProfile()->NavmeshRelativeDirectory, trimChars, 2)
							 + "/"
							 + CurrentMapName
							 + ".nav";

		saveAll(sanitisedPath.c_str());
	}

	if (imguiButton("Load"))
	{
		dtFreeNavMesh(m_navMesh);
		dtFreeTileCache(m_tileCache);

		unsigned char trimChars[3] = "/\\";
		string sanitisedPath = trim(GetCurrentGameProfile()->GameDirectory, trimChars, 2)
			+ "/"
			+ trim(GetCurrentGameProfile()->NavmeshRelativeDirectory, trimChars, 2)
			+ "/"
			+ CurrentMapName
			+ ".nav";

		loadAll(sanitisedPath.c_str());
		m_navQuery->init(m_navMesh, 2048);
	}

	imguiUnindent();
	imguiUnindent();

	imguiSeparator();
}

void Sample_TempObstacles::handleTools()
{
	int type = !m_tool ? TOOL_NONE : m_tool->type();

	if (imguiCheck("Edit Map", type == TOOL_MESH_EDITOR))
	{
		setTool(new MeshEditorTool);
	}
	if (imguiCheck("Test Navmesh", type == TOOL_NAVMESH_TESTER))
	{
		setTool(new NavMeshTesterTool);
	}
	if (imguiCheck("Highlight Tile Cache", type == TOOL_TILE_HIGHLIGHT))
	{
		setTool(new TempObstacleHilightTool);
	}
	if (imguiCheck("Create Temp Obstacles", type == TOOL_TEMP_OBSTACLE))
	{
		setTool(new TempObstacleCreateTool);
	}
	if (imguiCheck("Create Off-Mesh Links", type == TOOL_OFFMESH_CONNECTION))
	{
		setTool(new OffMeshConnectionTool);
	}
	if (imguiCheck("Create Convex Volumes", type == TOOL_CONVEX_VOLUME))
	{
		setTool(new ConvexVolumeTool);
	}
	if (imguiCheck("Place Nav Hints", type == TOOL_NAV_HINTS))
	{
		setTool(new NavHintTool);
	}

	imguiSeparatorLine();

	imguiIndent();

	if (m_tool)
		m_tool->handleMenu();

	imguiUnindent();
}

void Sample_TempObstacles::handleDebugMode()
{
	// Check which modes are valid.
	bool valid[MAX_DRAWMODE];
	for (int i = 0; i < MAX_DRAWMODE; ++i)
		valid[i] = false;

	if (m_geom)
	{
		dtNavMesh* CurrentMesh = m_NavMeshArray[m_SelectedNavMeshIndex].m_navMesh;
		dtNavMeshQuery* CurrentQuery = m_NavMeshArray[m_SelectedNavMeshIndex].m_navQuery;

		valid[DRAWMODE_NAVMESH] = CurrentMesh != 0;
		valid[DRAWMODE_NAVMESH_TRANS] = CurrentMesh != 0;
		valid[DRAWMODE_NAVMESH_BVTREE] = CurrentMesh != 0;
		valid[DRAWMODE_NAVMESH_NODES] = CurrentQuery != 0;
		valid[DRAWMODE_NAVMESH_PORTALS] = CurrentMesh != 0;
		valid[DRAWMODE_NAVMESH_INVIS] = CurrentMesh != 0;

		valid[DRAWMODE_MESH] = true;
		valid[DRAWMODE_CACHE_BOUNDS] = true;
	}

	int unavail = 0;
	for (int i = 0; i < MAX_DRAWMODE; ++i)
		if (!valid[i]) unavail++;

	if (unavail == MAX_DRAWMODE)
		return;

	imguiLabel("Draw");
	if (imguiCheck("Illusionary Surfaces", m_drawIllusionary, valid[DRAWMODE_MESH]))
		m_drawIllusionary = !m_drawIllusionary;
	if (imguiCheck("Input Mesh", m_drawMode == DRAWMODE_MESH, valid[DRAWMODE_MESH]))
		m_drawMode = DRAWMODE_MESH;
	if (imguiCheck("Navmesh", m_drawMode == DRAWMODE_NAVMESH, valid[DRAWMODE_NAVMESH]))
		m_drawMode = DRAWMODE_NAVMESH;
	if (imguiCheck("Navmesh Invis", m_drawMode == DRAWMODE_NAVMESH_INVIS, valid[DRAWMODE_NAVMESH_INVIS]))
		m_drawMode = DRAWMODE_NAVMESH_INVIS;
	if (imguiCheck("Navmesh Trans", m_drawMode == DRAWMODE_NAVMESH_TRANS, valid[DRAWMODE_NAVMESH_TRANS]))
		m_drawMode = DRAWMODE_NAVMESH_TRANS;
	if (imguiCheck("Navmesh BVTree", m_drawMode == DRAWMODE_NAVMESH_BVTREE, valid[DRAWMODE_NAVMESH_BVTREE]))
		m_drawMode = DRAWMODE_NAVMESH_BVTREE;
	if (imguiCheck("Navmesh Nodes", m_drawMode == DRAWMODE_NAVMESH_NODES, valid[DRAWMODE_NAVMESH_NODES]))
		m_drawMode = DRAWMODE_NAVMESH_NODES;
	if (imguiCheck("Navmesh Portals", m_drawMode == DRAWMODE_NAVMESH_PORTALS, valid[DRAWMODE_NAVMESH_PORTALS]))
		m_drawMode = DRAWMODE_NAVMESH_PORTALS;
	if (imguiCheck("Cache Bounds", m_drawMode == DRAWMODE_CACHE_BOUNDS, valid[DRAWMODE_CACHE_BOUNDS]))
		m_drawMode = DRAWMODE_CACHE_BOUNDS;

	if (unavail)
	{
		imguiValue("Tick 'Keep Intermediate Results'");
		imguiValue("rebuild some tiles to see");
		imguiValue("more debug mode options.");
	}
}

void Sample_TempObstacles::handleRender()
{
	if (!m_geom || !m_geom->getMesh())
		return;

	const float texScale = 1.0f / (m_cellSize * 10.0f);

	dtTileCache* CurrentTileCache = getTileCache();
	dtNavMesh* CurrentNavMesh = getNavMesh();
	dtNavMeshQuery* CurrentNavMeshQuery = getNavMeshQuery();

	// Draw mesh
	if (m_drawMode != DRAWMODE_NAVMESH_TRANS)
	{
		// Draw mesh
		duDebugDrawTriMeshSlope(&m_dd, m_geom->getMesh()->getVerts(), m_geom->getMesh()->getVertCount(),
								m_geom->getMesh()->getTris(), m_geom->getMesh()->getNormals(), m_geom->getMesh()->getTriCount(),
								m_agentMaxSlope, texScale, m_geom->getMesh()->getSurfaceTypes(), m_drawIllusionary);
		m_geom->drawOffMeshConnections(&m_dd);
	}

	if (CurrentTileCache && m_drawMode == DRAWMODE_CACHE_BOUNDS)
		drawTiles(&m_dd, CurrentTileCache);

	if (CurrentTileCache)
	{
		drawObstacles(&m_dd, CurrentTileCache);
	}


	glDepthMask(GL_FALSE);

	// Draw bounds
	const float* bmin = m_geom->getNavMeshBoundsMin();
	const float* bmax = m_geom->getNavMeshBoundsMax();
	duDebugDrawBoxWire(&m_dd, bmin[0],bmin[1],bmin[2], bmax[0],bmax[1],bmax[2], duRGBA(255,255,255,128), 1.0f);

	// Tiling grid.
	int gw = 0, gh = 0;
	rcCalcGridSize(bmin, bmax, m_cellSize, &gw, &gh);
	const int tw = (gw + (int)m_tileSize-1) / (int)m_tileSize;
	const int th = (gh + (int)m_tileSize-1) / (int)m_tileSize;
	const float s = m_tileSize*m_cellSize;
	duDebugDrawGridXZ(&m_dd, bmin[0],bmin[1],bmin[2], tw,th, s, duRGBA(0,0,0,64), 1.0f);

	if (CurrentNavMesh && CurrentNavMeshQuery &&
		(m_drawMode == DRAWMODE_NAVMESH ||
		 m_drawMode == DRAWMODE_NAVMESH_TRANS ||
		 m_drawMode == DRAWMODE_NAVMESH_BVTREE ||
		 m_drawMode == DRAWMODE_NAVMESH_NODES ||
		 m_drawMode == DRAWMODE_NAVMESH_PORTALS ||
		 m_drawMode == DRAWMODE_NAVMESH_INVIS))
	{
		if (m_drawMode != DRAWMODE_NAVMESH_INVIS)
			duDebugDrawNavMeshWithClosedList(&m_dd, *CurrentNavMesh, *CurrentNavMeshQuery, m_navMeshDrawFlags/*|DU_DRAWNAVMESH_COLOR_TILES*/);
		if (m_drawMode == DRAWMODE_NAVMESH_BVTREE)
			duDebugDrawNavMeshBVTree(&m_dd, *CurrentNavMesh);
		if (m_drawMode == DRAWMODE_NAVMESH_PORTALS)
			duDebugDrawNavMeshPortals(&m_dd, *CurrentNavMesh);
		if (m_drawMode == DRAWMODE_NAVMESH_NODES)
			duDebugDrawNavMeshNodes(&m_dd, *CurrentNavMeshQuery);
		duDebugDrawNavMeshPolysWithFlags(&m_dd, *CurrentNavMesh, SAMPLE_POLYFLAGS_DISABLED, duRGBA(0,0,0,128));
	}


	glDepthMask(GL_TRUE);

	m_geom->drawConvexVolumes(m_SelectedNavMeshIndex, &m_dd);

	if (m_tool)
		m_tool->handleRender();
	renderToolStates();

	glDepthMask(GL_TRUE);
}

void Sample_TempObstacles::renderCachedTile(const int tx, const int ty, const int type)
{
	if (m_tileCache)
		drawDetail(&m_dd,m_tileCache,tx,ty,type);
}

void Sample_TempObstacles::renderCachedTileOverlay(const int tx, const int ty, double* proj, double* model, int* view)
{
	if (m_tileCache)
		drawDetailOverlay(m_tileCache, tx, ty, proj, model, view);
}

void Sample_TempObstacles::handleRenderOverlay(double* proj, double* model, int* view)
{
	if (m_tool)
		m_tool->handleRenderOverlay(proj, model, view);
	renderOverlayToolStates(proj, model, view);

	// Stats
/*	imguiDrawRect(280,10,300,100,imguiRGBA(0,0,0,64));

	char text[64];
	int y = 110-30;

	snprintf(text,64,"Lean Data: %.1fkB", m_tileCache->getRawSize()/1024.0f);
	imguiDrawText(300, y, IMGUI_ALIGN_LEFT, text, imguiRGBA(255,255,255,255));
	y -= 20;

	snprintf(text,64,"Compressed: %.1fkB (%.1f%%)", m_tileCache->getCompressedSize()/1024.0f,
			 m_tileCache->getRawSize() > 0 ? 100.0f*(float)m_tileCache->getCompressedSize()/(float)m_tileCache->getRawSize() : 0);
	imguiDrawText(300, y, IMGUI_ALIGN_LEFT, text, imguiRGBA(255,255,255,255));
	y -= 20;

	if (m_rebuildTileCount > 0 && m_rebuildTime > 0.0f)
	{
		snprintf(text,64,"Changed obstacles, rebuild %d tiles: %.3f ms", m_rebuildTileCount, m_rebuildTime);
		imguiDrawText(300, y, IMGUI_ALIGN_LEFT, text, imguiRGBA(255,192,0,255));
		y -= 20;
	}
	*/
}

void Sample_TempObstacles::handleMeshChanged(class InputGeom* geom)
{
	Sample::handleMeshChanged(geom);

	dtFreeTileCache(m_tileCache);
	m_tileCache = 0;

	dtFreeNavMesh(m_navMesh);
	m_navMesh = 0;

	if (m_tool)
	{
		m_tool->reset();
		m_tool->init(this);
		m_tmproc->init(m_geom);
	}
	resetToolStates();
	initToolStates(this);
}

void Sample_TempObstacles::addTempObstacle(const float* pos, const unsigned char Area)
{
	dtTileCache* CurrentTileCache = m_NavMeshArray[m_SelectedNavMeshIndex].m_tileCache;

	if (!CurrentTileCache)
		return;
	float p[3];
	dtVcopy(p, pos);
	p[1] -= 0.5f;
	CurrentTileCache->addObstacle(p, 32.0f, 100.0f, Area, 0);
}

void Sample_TempObstacles::removeTempObstacle(const float* sp, const float* sq)
{
	dtTileCache* CurrentTileCache = m_NavMeshArray[m_SelectedNavMeshIndex].m_tileCache;

	if (!CurrentTileCache)
		return;
	dtObstacleRef ref = hitTestObstacle(CurrentTileCache, sp, sq);
	CurrentTileCache->removeObstacle(ref);
}

void Sample_TempObstacles::clearAllTempObstacles()
{
	if (!m_tileCache)
		return;
	for (int i = 0; i < m_tileCache->getObstacleCount(); ++i)
	{
		const dtTileCacheObstacle* ob = m_tileCache->getObstacle(i);
		if (ob->state == DT_OBSTACLE_EMPTY) continue;
		m_tileCache->removeObstacle(m_tileCache->getObstacleRef(ob));
	}
}

bool Sample_TempObstacles::handleBuild()
{
	dtStatus status;

	if (!m_geom || !m_geom->getMesh())
	{
		m_ctx->log(RC_LOG_ERROR, "buildTiledNavigation: No vertices and triangles.");
		return false;
	}

	m_geom->rebuildChunkyTriMesh();

	m_tmproc->init(m_geom);

	// Init cache
	const float* bmin = m_geom->getNavMeshBoundsMin();
	const float* bmax = m_geom->getNavMeshBoundsMax();
	int gw = 0, gh = 0;
	rcCalcGridSize(bmin, bmax, m_cellSize, &gw, &gh);
	const int ts = (int)m_tileSize;
	const int tw = (gw + ts - 1) / ts;
	const int th = (gh + ts - 1) / ts;

	int navmeshMemUsage = 0;

	vector<NavMeshDefinition> AllNavMeshes = GetAllMeshDefinitions();
	unsigned int MeshIndex = 0;

	for (auto it = AllNavMeshes.begin(); it != AllNavMeshes.end(); it++)
	{
		NavMeshEntry* meshDefinition = &m_NavMeshArray[MeshIndex];

		if (!meshDefinition->m_navQuery)
		{
			meshDefinition->m_navQuery = dtAllocNavMeshQuery();
		}

		std::vector <dtOffMeshConnection> ConnectionsToReadd;

		if (meshDefinition->m_tileCache)
		{
			for (int i = 0; i < meshDefinition->m_tileCache->getOffMeshCount(); i++)
			{
				const dtOffMeshConnection* con = meshDefinition->m_tileCache->getOffMeshConnection(i);

				if (con->state == DT_OFFMESH_EMPTY || con->state == DT_OFFMESH_REMOVING) { continue; }

				ConnectionsToReadd.push_back(*con);
			}
		}

		// Generation params.
		rcConfig cfg;
		memset(&cfg, 0, sizeof(cfg));
		cfg.cs = m_cellSize;
		cfg.ch = m_cellHeight;
		cfg.walkableSlopeAngle = it->MaxSlope;
		cfg.walkableHeight = (int)ceilf(it->AgentStandingHeight / cfg.ch);
		cfg.crouchHeight = (int)ceilf(it->AgentCrouchingHeight / cfg.ch);
		cfg.walkableClimb = (int)floorf(it->MaxStep / cfg.ch);
		cfg.walkableRadius = (int)ceilf(it->AgentRadius / cfg.cs);
		cfg.maxEdgeLen = (int)(m_edgeMaxLen / m_cellSize);
		cfg.maxSimplificationError = m_edgeMaxError;
		cfg.minRegionArea = (int)rcSqr(m_regionMinSize);		// Note: area = size*size
		cfg.mergeRegionArea = (int)rcSqr(m_regionMergeSize);	// Note: area = size*size
		cfg.maxVertsPerPoly = (int)m_vertsPerPoly;
		cfg.tileSize = (int)m_tileSize;
		cfg.borderSize = cfg.walkableRadius + 3; // Reserve enough padding.
		cfg.width = cfg.tileSize + cfg.borderSize * 2;
		cfg.height = cfg.tileSize + cfg.borderSize * 2;
		cfg.detailSampleDist = m_detailSampleDist < 0.9f ? 0 : m_cellSize * m_detailSampleDist;
		cfg.detailSampleMaxError = m_cellHeight * m_detailSampleMaxError;
		rcVcopy(cfg.bmin, bmin);
		rcVcopy(cfg.bmax, bmax);

		// Tile cache params.
		dtTileCacheParams tcparams;
		memset(&tcparams, 0, sizeof(tcparams));
		rcVcopy(tcparams.orig, bmin);
		tcparams.cs = m_cellSize;
		tcparams.ch = m_cellHeight;
		tcparams.width = (int)m_tileSize;
		tcparams.height = (int)m_tileSize;
		tcparams.walkableHeight = it->AgentStandingHeight;
		tcparams.crouchHeight = it->AgentCrouchingHeight;
		tcparams.walkableRadius = it->AgentRadius;
		tcparams.walkableClimb = it->MaxStep;
		tcparams.maxSimplificationError = m_edgeMaxError;
		tcparams.maxTiles = tw * th * EXPECTED_LAYERS_PER_TILE;
		tcparams.maxObstacles = 128;
		tcparams.maxOffMeshConnections = 512;

		dtFreeTileCache(meshDefinition->m_tileCache);

		meshDefinition->m_tileCache = dtAllocTileCache();
		if (!meshDefinition->m_tileCache)
		{
			m_ctx->log(RC_LOG_ERROR, "buildTiledNavigation: Could not allocate tile cache.");
			return false;
		}
		status = meshDefinition->m_tileCache->init(&tcparams, m_talloc, m_tcomp, m_tmproc);
		if (dtStatusFailed(status))
		{
			m_ctx->log(RC_LOG_ERROR, "buildTiledNavigation: Could not init tile cache.");
			return false;
		}

		dtFreeNavMesh(meshDefinition->m_navMesh);

		meshDefinition->m_navMesh = dtAllocNavMesh();
		if (!meshDefinition->m_navMesh)
		{
			m_ctx->log(RC_LOG_ERROR, "buildTiledNavigation: Could not allocate navmesh.");
			return false;
		}

		dtNavMeshParams params;
		memset(&params, 0, sizeof(params));
		rcVcopy(params.orig, bmin);
		params.tileWidth = m_tileSize * m_cellSize;
		params.tileHeight = m_tileSize * m_cellSize;
		params.maxTiles = m_maxTiles;
		params.maxPolys = m_maxPolysPerTile;

		status = meshDefinition->m_navMesh->init(&params);
		if (dtStatusFailed(status))
		{
			m_ctx->log(RC_LOG_ERROR, "buildTiledNavigation: Could not init navmesh.");
			return false;
		}

		status = meshDefinition->m_navQuery->init(meshDefinition->m_navMesh, 2048);
		if (dtStatusFailed(status))
		{
			m_ctx->log(RC_LOG_ERROR, "buildTiledNavigation: Could not init Detour navmesh query");
			return false;
		}


		// Preprocess tiles.

		m_ctx->resetTimers();

		m_cacheLayerCount = 0;
		m_cacheCompressedSize = 0;
		m_cacheRawSize = 0;

		for (int y = 0; y < th; ++y)
		{
			for (int x = 0; x < tw; ++x)
			{
				TileCacheData tiles[MAX_LAYERS];
				memset(tiles, 0, sizeof(tiles));
				int ntiles = rasterizeTileLayers(MeshIndex, x, y, cfg, tiles, MAX_LAYERS);

				for (int i = 0; i < ntiles; ++i)
				{
					TileCacheData* tile = &tiles[i];
					status = meshDefinition->m_tileCache->addTile(tile->data, tile->dataSize, DT_COMPRESSEDTILE_FREE_DATA, 0);
					if (dtStatusFailed(status))
					{
						dtFree(tile->data);
						tile->data = 0;
						continue;
					}

					m_cacheLayerCount++;
					m_cacheCompressedSize += tile->dataSize;
					m_cacheRawSize += calcLayerBufferSize(tcparams.width, tcparams.height);
				}
			}
		}

		// Build initial meshes
		m_ctx->startTimer(RC_TIMER_TOTAL);
		for (int y = 0; y < th; ++y)
			for (int x = 0; x < tw; ++x)
				meshDefinition->m_tileCache->buildNavMeshTilesAt(x, y, meshDefinition->m_navMesh);
		m_ctx->stopTimer(RC_TIMER_TOTAL);

		if (meshDefinition->m_tileCache)
		{
			for (auto it = ConnectionsToReadd.begin(); it != ConnectionsToReadd.end(); it++)
			{
				meshDefinition->m_tileCache->addOffMeshConnection(&it->pos[0], &it->pos[3], it->rad, it->area, it->flags, it->bBiDir, 0);
			}
		}

		if (meshDefinition->m_navMesh)
		{
			const dtNavMesh* cNavMesh = meshDefinition->m_navMesh;

			for (int i = 0; i < cNavMesh->getMaxTiles(); ++i)
			{
				const dtMeshTile* tile = cNavMesh->getTile(i);
				if (tile->header)
					navmeshMemUsage += tile->dataSize;
			}
		}

		MeshIndex++;
	}

	m_cacheBuildTimeMs = m_ctx->getAccumulatedTime(RC_TIMER_TOTAL)/1000.0f;
	m_cacheBuildMemUsage = static_cast<unsigned int>(m_talloc->high);

	printf("navmeshMemUsage = %.1f kB", navmeshMemUsage/1024.0f);


	if (m_tool)
		m_tool->init(this);
	initToolStates(this);



	return true;
}

void Sample_TempObstacles::handleUpdate(const float dt)
{
	Sample::handleUpdate(dt);

	int NumMeshes = GetNumNavMeshes();

	for (int i = 0; i < NumMeshes; i++)
	{

		if (!m_NavMeshArray[i].m_navMesh)
			return;
		if (!m_NavMeshArray[i].m_tileCache)
			return;

		m_NavMeshArray[i].m_tileCache->update(dt, m_NavMeshArray[i].m_navMesh);
	}
}

void Sample_TempObstacles::getTilePos(const float* pos, int& tx, int& ty)
{
	if (!m_geom) return;

	const float* bmin = m_geom->getNavMeshBoundsMin();

	const float ts = m_tileSize*m_cellSize;
	tx = (int)((pos[0] - bmin[0]) / ts);
	ty = (int)((pos[2] - bmin[2]) / ts);
}

static const int TILECACHESET_MAGIC = 'T'<<24 | 'S'<<16 | 'E'<<8 | 'T'; //'TSET';
static const int TILECACHESET_VERSION = 4;

struct TileCacheSetHeader
{
	int magic = 0;
	int version = 0;
	int numTiles = 0;
	dtNavMeshParams meshParams;
	dtTileCacheParams cacheParams;

	int NumOffMeshCons = 0;
	int OffMeshConsOffset = 0;

	int NumConvexVols = 0;
	int ConvexVolsOffset = 0;

	int NumNavHints = 0;
	int NavHintsOffset = 0;
};

struct TileCacheExportHeader
{
	int magic;
	int version;

	int numTileCaches;
	int tileCacheDataOffset = 0;

	int tileCacheOffsets[8];

	int NumSurfTypes;
	int SurfTypesOffset;
};

struct TileCacheTileHeader
{
	dtCompressedTileRef tileRef;
	int dataSize;
};

void Sample_TempObstacles::saveAll(const char* path)
{
	if (!m_NavMeshArray[0].m_tileCache) return;

	SaveData(path);
}

void Sample_TempObstacles::SaveData(const char* path)
{
	if (!m_NavMeshArray[0].m_tileCache) return;

	FILE* fp = fopen(path, "wb");
	if (!fp)
		return;

	int NumMeshes = GetNumNavMeshes();

	TileCacheExportHeader NewFileHeader;
	NewFileHeader.magic = TILECACHESET_MAGIC;
	NewFileHeader.version = TILECACHESET_VERSION;
	NewFileHeader.numTileCaches = NumMeshes;
	NewFileHeader.NumSurfTypes = m_geom->getSurfaceTypeCount();
	memset(NewFileHeader.tileCacheOffsets, 0, sizeof(NewFileHeader.tileCacheOffsets));

	fwrite(&NewFileHeader, sizeof(TileCacheExportHeader), 1, fp);

	NewFileHeader.SurfTypesOffset = ftell(fp);

	const int* surfTypes = m_geom->getSurfaceTypes();
	int surfTypesSize = NewFileHeader.NumSurfTypes * sizeof(const int);

	fwrite(surfTypes, surfTypesSize, 1, fp);

	NewFileHeader.tileCacheDataOffset = ftell(fp);

	for (int i = 0; i < NumMeshes; i++)
	{
		NewFileHeader.tileCacheOffsets[i] = ftell(fp);

		TileCacheSetHeader tcHeader;
		tcHeader.magic = TILECACHESET_MAGIC;
		tcHeader.version = TILECACHESET_VERSION;
		tcHeader.NumOffMeshCons = 0;

		// First remove all the connections so they don't get persisted. These are meant to be dynamic and not baked into the saved data
		for (int ii = 0; ii < m_NavMeshArray[i].m_tileCache->getOffMeshCount(); ii++)
		{
			dtOffMeshConnection* con = m_NavMeshArray[i].m_tileCache->getOffMeshConnection(ii);

			if (con->state == DT_OFFMESH_EMPTY || con->state == DT_OFFMESH_REMOVING) { continue; }

			m_NavMeshArray[i].m_navMesh->unconnectOffMeshLink(con);

			con->state = DT_OFFMESH_DIRTY;
			tcHeader.NumOffMeshCons++;
		}

		const ConvexVolume* vols = m_geom->getConvexVolumes();

		for (int ii = 0; ii < m_geom->getConvexVolumeCount(); ii++)
		{
			if (vols[i].NavMeshIndex != i) { continue; }

			tcHeader.NumConvexVols++;
		}

		const NavHint* hints = m_geom->getNavHints();

		for (int ii = 0; ii < m_geom->getNavHintCount(); ii++)
		{
			if (hints[i].NavMeshIndex != i) { continue; }

			tcHeader.NumNavHints++;
		}

		for (int ii = 0; ii < m_NavMeshArray[i].m_tileCache->getTileCount(); ++ii)
		{
			const dtCompressedTile* tile = m_NavMeshArray[i].m_tileCache->getTile(ii);
			if (!tile || !tile->header || !tile->dataSize) continue;
			tcHeader.numTiles++;
		}

		memcpy(&tcHeader.cacheParams, m_NavMeshArray[i].m_tileCache->getParams(), sizeof(dtTileCacheParams));
		memcpy(&tcHeader.meshParams, m_NavMeshArray[i].m_navMesh->getParams(), sizeof(dtNavMeshParams));

		fwrite(&tcHeader, sizeof(TileCacheSetHeader), 1, fp);

		// Store tiles.
		for (int ii = 0; ii < m_NavMeshArray[i].m_tileCache->getTileCount(); ++ii)
		{
			const dtCompressedTile* tile = m_NavMeshArray[i].m_tileCache->getTile(ii);
			if (!tile || !tile->header || !tile->dataSize) continue;

			TileCacheTileHeader tileHeader;
			tileHeader.tileRef = m_NavMeshArray[i].m_tileCache->getTileRef(tile);
			tileHeader.dataSize = tile->dataSize;
			fwrite(&tileHeader, sizeof(tileHeader), 1, fp);

			fwrite(tile->data, tile->dataSize, 1, fp);
		}

		tcHeader.OffMeshConsOffset = ftell(fp);

		// First remove all the connections so they don't get persisted. These are meant to be dynamic and not baked into the saved data
		for (int ii = 0; ii < m_NavMeshArray[i].m_tileCache->getOffMeshCount(); ii++)
		{
			dtOffMeshConnection* con = m_NavMeshArray[i].m_tileCache->getOffMeshConnection(ii);

			if (con->state == DT_OFFMESH_EMPTY || con->state == DT_OFFMESH_REMOVING) { continue; }

			fwrite(con, sizeof(dtOffMeshConnection), 1, fp);
		}

		tcHeader.ConvexVolsOffset = ftell(fp);

		for (int ii = 0; ii < m_geom->getConvexVolumeCount(); ii++)
		{
			if (vols[i].NavMeshIndex != i) { continue; }

			fwrite(&vols[i], sizeof(ConvexVolume), 1, fp);
		}

		tcHeader.NavHintsOffset = ftell(fp);

		for (int ii = 0; ii < m_geom->getNavHintCount(); ii++)
		{
			if (hints[i].NavMeshIndex != i) { continue; }

			fwrite(&hints[i], sizeof(NavHint), 1, fp);
		}

		int endMeshOffset = ftell(fp);

		fseek(fp, NewFileHeader.tileCacheOffsets[i], SEEK_SET);
		fwrite(&tcHeader, sizeof(TileCacheSetHeader), 1, fp);
		fseek(fp, endMeshOffset, SEEK_SET);

	}

	fseek(fp, 0, SEEK_SET);
	fwrite(&NewFileHeader, sizeof(TileCacheExportHeader), 1, fp);

	fclose(fp);
}

void Sample_TempObstacles::loadAll(const char* path)
{
	FILE* fp = fopen(path, "rb");
	if (!fp) return;


	// Read header.
	TileCacheExportHeader fileHeader;
	size_t headerReadReturnCode = fread(&fileHeader, sizeof(TileCacheExportHeader), 1, fp);
	if( headerReadReturnCode != 1)
	{
		// Error or early EOF
		fclose(fp);
		return;
	}
	if (fileHeader.magic != TILECACHESET_MAGIC)
	{
		fclose(fp);
		return;
	}
	if (fileHeader.version != TILECACHESET_VERSION)
	{
		fclose(fp);
		return;
	}

	fseek(fp, fileHeader.SurfTypesOffset, SEEK_SET);

	int surfTypesSize = fileHeader.NumSurfTypes * sizeof(int);

	int* surfTypes = (int*)malloc(surfTypesSize);

	fread(surfTypes, surfTypesSize, 1, fp);

	for (int i = 0; i < fileHeader.NumSurfTypes; i++)
	{
		m_geom->SetTriangleArea(i, surfTypes[i]);
	}

	for (int i = 0; i < fileHeader.numTileCaches; i++)
	{
		fseek(fp, fileHeader.tileCacheOffsets[i], SEEK_SET);

		dtFreeNavMesh(m_NavMeshArray[i].m_navMesh);
		dtFreeTileCache(m_NavMeshArray[i].m_tileCache);
		dtFreeNavMeshQuery(m_NavMeshArray[i].m_navQuery);

		TileCacheSetHeader tcHeader;

		size_t headerReadReturnCode = fread(&tcHeader, sizeof(TileCacheSetHeader), 1, fp);
		if (headerReadReturnCode != 1)
		{
			// Error or early EOF
			continue;
		}

		m_NavMeshArray[i].m_navMesh = dtAllocNavMesh();
		if (!m_NavMeshArray[i].m_navMesh) { continue; }

		m_NavMeshArray[i].m_tileCache = dtAllocTileCache();
		if (!m_NavMeshArray[i].m_tileCache) { continue; }

		m_NavMeshArray[i].m_navQuery = dtAllocNavMeshQuery();
		if (!m_NavMeshArray[i].m_navQuery) { continue; }

		dtStatus status = m_NavMeshArray[i].m_navMesh->init(&tcHeader.meshParams);
		if (dtStatusFailed(status)) { continue; }

		 status = m_NavMeshArray[i].m_tileCache->init(&tcHeader.cacheParams, m_talloc, m_tcomp, m_tmproc);
		if (dtStatusFailed(status)) { continue; }

		// Read tiles.
		for (int ii = 0; ii < tcHeader.numTiles; ++ii)
		{
			TileCacheTileHeader tileHeader;
			size_t tileHeaderReadReturnCode = fread(&tileHeader, sizeof(tileHeader), 1, fp);
			if (tileHeaderReadReturnCode != 1) { continue; }

			if (!tileHeader.tileRef || !tileHeader.dataSize)
				break;

			unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
			if (!data) break;
			memset(data, 0, tileHeader.dataSize);
			size_t tileDataReadReturnCode = fread(data, tileHeader.dataSize, 1, fp);
			if (tileDataReadReturnCode != 1)
			{
				// Error or early EOF
				dtFree(data);
				fclose(fp);
				return;
			}

			dtCompressedTileRef tile = 0;
			dtStatus addTileStatus = m_NavMeshArray[i].m_tileCache->addTile(data, tileHeader.dataSize, DT_COMPRESSEDTILE_FREE_DATA, &tile);
			if (dtStatusFailed(addTileStatus))
			{
				dtFree(data);
			}

			if (tile)
				m_NavMeshArray[i].m_tileCache->buildNavMeshTile(tile, m_NavMeshArray[i].m_navMesh);
		}

		m_NavMeshArray[i].m_navQuery->init(m_NavMeshArray[i].m_navMesh, 2048);

		for (int ii = 0; ii < tcHeader.NumOffMeshCons; ii++)
		{
			dtOffMeshConnection def;

			fread(&def, sizeof(dtOffMeshConnection), 1, fp);

			m_NavMeshArray[i].m_tileCache->addOffMeshConnection(&def.pos[0], &def.pos[3], 10.0f, def.area, def.flags, def.bBiDir, 0);
		}

		for (int ii = 0; ii < tcHeader.NumConvexVols; ii++)
		{
			ConvexVolume def;

			fread(&def, sizeof(ConvexVolume), 1, fp);

			m_geom->addConvexVolume(i, def.verts, def.nverts, def.hmin, def.hmax, def.area);
		}

		for (int ii = 0; ii < tcHeader.NumNavHints; ii++)
		{
			NavHint def;

			fread(&def, sizeof(NavHint), 1, fp);

			m_geom->addNavHint(i, def.position, def.hintType);
		}

	}

	fclose(fp);
}

void Sample_TempObstacles::addOffMeshConnection(const float* spos, const float* epos, const float rad, const unsigned char area, const unsigned int flags, const bool bBiDirectional)
{
	dtTileCache* CurrentTileCache = getTileCache();

	if (CurrentTileCache)
		CurrentTileCache->addOffMeshConnection(spos, epos, rad, area, flags, bBiDirectional, 0);
}

void Sample_TempObstacles::drawOffMeshConnections(duDebugDraw* dd)
{
	dtTileCache* CurrentTileCache = getTileCache();

	if (!CurrentTileCache) { return; }

	unsigned int conColor = duRGBA(192, 0, 128, 192);
	unsigned int baseColor = duRGBA(0, 0, 0, 64);

	dd->depthMask(false);

	dd->begin(DU_DRAW_LINES, 2.0f);

	// Draw obstacles
	for (int i = 0; i < CurrentTileCache->getOffMeshCount(); ++i)
	{
		const dtOffMeshConnection* con = CurrentTileCache->getOffMeshConnection(i);
		if (con->state == DT_OFFMESH_EMPTY || con->state == DT_OFFMESH_REMOVING) continue;

		unsigned int thisConColor = conColor;

		NavFlagDefinition* FlagDef = GetFlagByFlagId(con->flags);

		if (FlagDef)
		{
			thisConColor = FlagDef->DebugColor;
		}

		dd->vertex(con->pos[0], con->pos[1], con->pos[2], baseColor);
		dd->vertex(con->pos[0], con->pos[1] + 0.2f, con->pos[2], baseColor);

		dd->vertex(con->pos[3], con->pos[4], con->pos[5], baseColor);
		dd->vertex(con->pos[3], con->pos[4] + 0.2f, con->pos[5], baseColor);

		duAppendCircle(dd, con->pos[0], con->pos[1] + 0.1f, con->pos[2], con->rad, baseColor);
		duAppendCircle(dd, con->pos[3], con->pos[4] + 0.1f, con->pos[5], con->rad, baseColor);

		duAppendArc(dd, con->pos[0], con->pos[1], con->pos[2], con->pos[3], con->pos[4], con->pos[5], 0.25f,
			(con->bBiDir) ? 0.6f : 0.0f, 0.6f, thisConColor);
	}

	dd->end();

	dd->depthMask(true);
}

dtOffMeshConnectionRef hitTestOffMeshConnection(const dtTileCache* tc, const float* pos)
{
	float tmin = FLT_MAX;
	const dtOffMeshConnection* conmin = 0;
	for (int i = 0; i < tc->getOffMeshCount(); ++i)
	{
		const dtOffMeshConnection* con = tc->getOffMeshConnection(i);
		if (con->state == DT_OFFMESH_EMPTY)
			continue;

		float distSpos = dtVdistSqr(pos, &con->pos[0]);
		float distEpos = dtVdistSqr(pos, &con->pos[3]);

		float thisDist = dtMin(distSpos, distEpos);

		if (thisDist > dtSqr(con->rad)) { continue; }

		if (thisDist < tmin)
		{
			conmin = con;
			tmin = thisDist;
		}
	}
	return tc->getOffMeshRef(conmin);
}

void Sample_TempObstacles::removeOffMeshConnection(const float* pos)
{
	dtTileCache* CurrentTileCache = getTileCache();

	if (CurrentTileCache)
	{
		dtOffMeshConnectionRef ref = hitTestOffMeshConnection(CurrentTileCache, pos);
		CurrentTileCache->removeOffMeshConnection(ref);
	}
}