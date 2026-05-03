/**
 * api_handlers.h
 * REST API route registration.
 *
 * Endpoints
 * ---------
 * GET  /api/session           – current session status
 * POST /api/session/start     – start a session  { "class_name": "..." }
 * POST /api/session/stop      – stop the session
 * GET  /api/students          – list registered students
 * POST /api/students          – register student
 * DELETE /api/students/:id    – remove student by index
 * GET  /api/attendance        – list all attendance records
 * GET  /api/attendance/session – attendance for the current session only
 * GET  /api/scan-logs         – recent raw scan events
 * GET  /api/stats             – summary statistics
 * POST /api/students/import   – bulk import students (JSON array)
 *
 * All mutation endpoints require: Authorization: Bearer <AUTH_TOKEN>
 */

#pragma once

#include <ESPAsyncWebServer.h>

namespace ApiHandlers {

/**
 * Register all REST routes and the static file handler on the given server.
 * Call once from setup() after Storage::begin() and Attendance::init().
 */
void registerRoutes(AsyncWebServer& server);

} // namespace ApiHandlers
