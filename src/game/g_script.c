/*
===========================================================================
Copyright (C) 2008 Maurice Doison

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

static int game_command( scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure )
{
  trap_SendConsoleCommand(EXEC_APPEND, SC_StringToChar(in[0].data.string));
  return 0;
}

static int game_cp( scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure )
{
  trap_SendServerCommand(-1, va("cp \"%s\"", SC_StringToChar(in[0].data.string)));
  return 0;
}

static scLibFunction_t game_lib[] = {
  { "command", "", game_command, { TYPE_STRING, TYPE_UNDEF }, TYPE_UNDEF, { .v = NULL } },
  { "cp", "", game_cp, { TYPE_STRING, TYPE_UNDEF }, TYPE_UNDEF, { .v = NULL } },
  { "" }
};

/*
======================================================================

Entity

======================================================================
*/

scClass_t *entity_class;

struct entity_struct_s
{
  gentity_t         *entity;
  scDataTypeHash_t  *userdata;
};

enum ent_closures_e
{
  // Vectors
  ENTITY_ORIGIN,
  // Members
  ENTITY_CLIENT,
  ENTITY_PARENT,
  ENTITY_NEXTTRAIN,
  ENTITY_PREVTRAIN,
  ENTITY_POS1,
  ENTITY_POS2,
  ENTITY_CLIPBRUSH,
  ENTITY_TARGETENT,
  ENTITY_MOVEDIR,
  ENTITY_OLDVELOCITY,
  ENTITY_ACCELERATION,
  ENTITY_OLDACCEL,
  ENTITY_JERK,
  ENTITY_CHAIN,
  ENTITY_ENEMY,
  ENTITY_ACTIVATOR,
  ENTITY_TEAMCHAIN,
  ENTITY_TEAMMASTER,
  ENTITY_PARENTNODE,
  ENTITY_OVERMINDNODE,
  ENTITY_TARGETED,
  ENTITY_TURRETAIM,
  ENTITY_TURRETAIMRATE,
  ENTITY_ANIMATION,
  ENTITY_BUILDER,
  // Methods
  ENTITY_LINK,
  // Class methods
  ENTITY_FIND,
};

gentity_t *G_EntityFromScript(scObject_t *object)
{
  struct entity_struct_s *cs;
  cs = object->data.data.userdata;

  return cs->entity;
}

scObject_t *G_GetScriptingEntity(gentity_t *entity)
{
  struct entity_struct_s *cs;

  // If there is no scripting entity for client, create it
  if(!entity->scriptingEntity)
  {
    entity->scriptingEntity = SC_ObjectNew(entity_class);
    entity->scriptingEntity->data.type = TYPE_USERDATA;
    cs = BG_Alloc(sizeof(struct entity_struct_s));
    entity->scriptingEntity->data.data.userdata = cs;

    cs->entity = entity;
    cs->userdata = SC_HashNew();

    // One object reference is saved to entity struct, so increase GC
    SC_ObjectGCInc(entity->scriptingEntity);
  }

  return entity->scriptingEntity;
}

static int entity_field_set(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  size_t addr = closure.s;
  gentity_t *entity = G_EntityFromScript(object);

  switch(in[1].type)
  {
    case TYPE_BOOLEAN:
      *(qboolean*)((void*)entity + addr) = in[1].data.boolean;
      break;

    case TYPE_INTEGER:
      *(int*)((void*)entity + addr) = in[1].data.integer;
      break;

    case TYPE_FLOAT:
      *(float*)((void*)entity + addr) = in[1].data.floating;
      break;

    case TYPE_STRING:
      // FIXME: /!\ Memory leak /!\ */
      SC_StringGCInc(in[1].data.string);
      *(const char**)((void*)entity + addr) = SC_StringToChar(in[1].data.string);
      break;

    default:
      SC_EXEC_ERROR(va("Internal error: unknow case in `entity_set', %s (%d)\n", __FILE__, __LINE__));
  }

  out[0].type = TYPE_UNDEF;
  return 0;
}

static int entity_field_get(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  size_t addr = closure.s;
  gentity_t *entity = G_EntityFromScript(object);

  switch(out[0].type)
  {
    case TYPE_BOOLEAN:
      out[0].data.boolean = *(qboolean*)((void*)entity + addr);
      break;

    case TYPE_INTEGER:
      out[0].data.integer = *(int*)((void*)entity + addr);
      break;

    case TYPE_FLOAT:
      out[0].data.floating = *(float*)((void*)entity + addr);
      break;

    case TYPE_STRING:
      out[0].data.string = SC_StringNewFromChar(*(char**)((void*)entity + addr));
      break;

    default:
      SC_EXEC_ERROR(va("Internal error: unknow case in `entity_set', %s (%d)\n", __FILE__, __LINE__));
  }

  return 0;
}

static int entity_object_vec3_set(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  int addr = closure.n;
  gentity_t *entity = G_EntityFromScript(object);

  memcpy(*(vec3_t**)((void*)entity + addr), (vec3_t*) in[1].data.object->data.data.userdata, sizeof(vec3_t));

  return 0;
}

static int entity_object_vec3_get(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  int addr = closure.n;
  gentity_t *entity = G_EntityFromScript(object);

  out[0].type = TYPE_OBJECT;
  out[0].data.object = SC_Vec3FromVec3_t(*(float**)((void*)entity + addr));

  return 0;
}

static int entity_object_vec4_set(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  int addr = closure.n;
  gentity_t *entity = G_EntityFromScript(object);

  memcpy(*(vec4_t**)((void*)entity + addr), (vec4_t*) in[1].data.object->data.data.userdata, sizeof(vec4_t));

  out[0].type = TYPE_UNDEF;
  return 0;
}

static int entity_object_vec4_get(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  int addr = closure.n;
  gentity_t *entity = G_EntityFromScript(object);

  out[0].type = TYPE_OBJECT;
  out[0].data.object = SC_Vec4FromVec4_t(*(float**)((void*)entity + addr));

  out[1].type = TYPE_UNDEF;
  return 0;
}

static int entity_object_entity_set(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  int addr = closure.n;
  gentity_t *entity = G_EntityFromScript(object);

  *(gentity_t**)((void*)entity + addr) = (gentity_t*) G_EntityFromScript(in[1].data.object->data.data.userdata);

  out[0].type = TYPE_UNDEF;
  return 0;
}

static int entity_object_entity_get(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  int addr = closure.n;
  gentity_t *entity = G_EntityFromScript(object);

  out[0].type = TYPE_OBJECT;
  out[0].data.object = G_GetScriptingEntity(*(gentity_t**)((void*)entity + addr));

  out[1].type = TYPE_UNDEF;
  return 0;
}

static int entity_object_client_set(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  int addr = closure.n;
  gentity_t *entity = G_EntityFromScript(object);

  *(gclient_t**)((void*)entity + addr) = (gclient_t*) G_EntityFromScript(in[1].data.object);

  out[0].type = TYPE_UNDEF;
  return 0;
}

static int entity_object_client_get(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  int addr = closure.n;
  gentity_t *entity = G_EntityFromScript(object);

  out[0].type = TYPE_OBJECT;
  out[0].data.object = G_GetScriptingClient(*(gclient_t**)((void*)entity + addr));

  out[1].type = TYPE_UNDEF;
  return 0;
}

static int entity_metaset(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  struct entity_struct_s *cs = object->data.data.userdata;

  SC_HashSet(cs->userdata, SC_StringToChar(in[1].data.string), &in[2]);
  return 0;
}

static int entity_metaget(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  struct entity_struct_s *cs = object->data.data.userdata;

  SC_HashGet(cs->userdata, SC_StringToChar(in[1].data.string), out);
  return 0;
}

static int entity_objectMethod(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *self = in[0].data.object;
  gentity_t *entity = G_EntityFromScript(self);
  int type = closure.n;

  switch(type)
  {
    case ENTITY_LINK:
      trap_LinkEntity(entity);
      break;

    default:
      SC_EXEC_ERROR(va("Internal error: unknow case in `entity_objectMethod', %s (%d)\n", __FILE__, __LINE__));
  }

  out[0].type = TYPE_UNDEF;
  return 0;
}

static int entity_classMethod(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  //scClass_t *class = in[0].data.class;
  int type = closure.n;

  switch(type)
  {
    case ENTITY_FIND:
      {
        scDataTypeArray_t *storeArray = SC_ArrayNew();
        scDataTypeFunction_t *function = in[1].data.function;
        gentity_t *entity;
        scDataTypeValue_t value;
        int n = 0;
        scDataTypeValue_t sin[2];
        scDataTypeValue_t sout;

        function->argument[0] = TYPE_OBJECT;
        function->argument[1] = TYPE_UNDEF;
        function->return_type = TYPE_BOOLEAN;
        sin[0].type = TYPE_OBJECT;
        sin[1].type = TYPE_UNDEF;
        value.type = TYPE_OBJECT;

        for(entity = g_entities; entity < &g_entities[ level.num_entities ]; entity++)
        {
          sin[0].data.object = G_GetScriptingEntity(entity);
          if(SC_RunFunction(function, sin, &sout) == -1)
            SC_EXEC_ERROR(va("error in functin pattern: %s", SC_StringToChar(sout.data.string)));

          if(sout.data.boolean)
          {
            value.data.object = G_GetScriptingEntity(entity);
            SC_ArraySet(storeArray, n, &value);
            n++;
          }
        }

        out->type = TYPE_ARRAY;
        out->data.array = storeArray;
      }
      break;


    default:
      SC_EXEC_ERROR(va("Internal error: unknow case in `entity_objectMethod', %s (%d)\n", __FILE__, __LINE__));
  }

  return 0;
}

static int entity_destructor(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  struct entity_struct_s *cs = object->data.data.userdata;

  SC_HashGCDec(cs->userdata);

  out[0].type = TYPE_UNDEF;
  return 0;
}

static int entity_get_entity_number(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *self = in[0].data.object;
  gentity_t *entity = G_EntityFromScript(self);

  out[0].data.integer = g_entities - entity;
  return 0;
}

static scLibObjectMember_t entity_members[] = {
  { "entityNumber", "", TYPE_INTEGER, NULL, entity_get_entity_number, { .v = NULL } },
  // Entity State
  { "number", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.number ) } },
  { "eType", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.eType ) } },
  { "eFlags", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.eFlags ) } },
  /* TODO: need a "trajectory" API type */
 /* { "pos", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.pos ) } },
  { "apos", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.apos ) } },*/
  { "time", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.time ) } },
  { "time2", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.time2 ) } },
  { "origin", "", TYPE_OBJECT, entity_object_vec3_set, entity_object_vec3_get, { .s = FIELD_ADDRESS(gentity_t, s.origin ) } },
  { "origin2", "", TYPE_INTEGER, entity_object_vec3_set, entity_object_vec3_get, { .s = FIELD_ADDRESS(gentity_t, s.origin2 ) } },
  { "angles", "", TYPE_INTEGER, entity_object_vec3_set, entity_object_vec3_get, { .s = FIELD_ADDRESS(gentity_t, s.angles ) } },
  { "angles2", "", TYPE_INTEGER, entity_object_vec3_set, entity_object_vec3_get, { .s = FIELD_ADDRESS(gentity_t, s.angles2 ) } },
  { "otherEntityNum", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.otherEntityNum ) } },
  { "otherEntityNum2", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.otherEntityNum2 ) } },
  { "groundEntityNum", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.groundEntityNum ) } },
  { "constantLight", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.constantLight ) } },
  { "loopSound", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.loopSound ) } },
  { "modelindex", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.modelindex ) } },
  { "modelindex2", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.modelindex2 ) } },
  { "clientNum", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.clientNum ) } },
  { "frame", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.frame ) } },
  { "solid", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.solid ) } },
  { "event", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.event ) } },
  { "eventParm", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.eventParm ) } },
  { "misc", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.misc ) } },
  { "weapon", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.weapon ) } },
  { "legsAnim", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.legsAnim ) } },
  { "torsoAnim", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.torsoAnim ) } },
  { "generic1", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, s.generic1 ) } },
  // Vectors
  // DEPRECATED { "origin",    "", TYPE_OBJECT,  entity_set, entity_get, { .s = ENTITY_ORIGIN },
  // TODO:  entityState_t s and entityShared r
  { "client", "", TYPE_OBJECT, entity_object_client_set, entity_object_client_get, { .s = FIELD_ADDRESS(gentity_t, client) } },
  { "inuse", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, inuse) } },
  { "classname", "", TYPE_STRING, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, classname) } },
  { "spawnflags", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, spawnflags) } },
  { "neverFree", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, neverFree) } },
  { "flags", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, flags) } },
  { "model", "", TYPE_STRING, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, model) } },
  { "model2", "", TYPE_STRING, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, model2) } },
  { "freetime", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, freetime) } },
  { "eventTime", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, eventTime) } },
  { "freeAfterEvent", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, freeAfterEvent) } },
  { "unlinkAfterEvent", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, unlinkAfterEvent) } },
  { "physicsObject", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, physicsObject) } },
  { "physicsBounce", "", TYPE_FLOAT, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, physicsBounce) } },
  { "clipmask", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, clipmask) } },
  /* TODO { "moverState", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, clipmask) } }, */
  { "soundPos1", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, soundPos1) } },
  { "sound1to2", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, sound1to2) } },
  { "sound2to1", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, sound2to1) } },
  { "soundPos2", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, soundPos2) } },
  { "soundLoop", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, soundLoop) } },
  { "parent", "", TYPE_OBJECT, entity_object_entity_set, entity_object_entity_get, { .s =  FIELD_ADDRESS(gentity_t, parent) } },
  { "nextTrain", "", TYPE_OBJECT, entity_object_entity_set, entity_object_entity_get, { .s =  FIELD_ADDRESS(gentity_t, nextTrain) } },
  { "prevTrain", "", TYPE_OBJECT, entity_object_entity_set, entity_object_entity_get, { .s =  FIELD_ADDRESS(gentity_t, prevTrain) } },
  { "pos1", "", TYPE_OBJECT, entity_object_vec3_set, entity_object_vec3_get, { .s =  FIELD_ADDRESS(gentity_t, pos1) } },
  { "pos2", "", TYPE_OBJECT, entity_object_vec3_set, entity_object_vec3_get, { .s =  FIELD_ADDRESS(gentity_t, pos2) } },
  { "rotatorAngle", "", TYPE_FLOAT, entity_field_set, entity_field_get, { .s =  FIELD_ADDRESS(gentity_t, rotatorAngle) } },
  { "clipBrush", "", TYPE_OBJECT, entity_object_entity_set, entity_object_entity_get, { .s =  FIELD_ADDRESS(gentity_t, clipBrush) } },
  { "message", "", TYPE_STRING, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, message) } },
  { "timestamp", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, timestamp) } },
  { "angle", "", TYPE_FLOAT, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, angle) } },
  { "target", "", TYPE_STRING, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, target) } },
  { "targetname", "", TYPE_STRING, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, targetname) } },
  { "team", "", TYPE_STRING, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, team) } },
  { "targetShaderName", "", TYPE_STRING, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, targetShaderName) } },
  { "targetShaderNewName", "", TYPE_STRING, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, targetShaderNewName) } },
  { "target_ent", "", TYPE_OBJECT, entity_object_entity_set, entity_object_entity_get, { .s =  FIELD_ADDRESS(gentity_t, target_ent) } },
  { "speed", "", TYPE_FLOAT, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, speed) } },
  { "lastSpeed", "", TYPE_FLOAT, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, lastSpeed) } },
  { "movedir", "", TYPE_OBJECT, entity_object_vec3_set, entity_object_vec3_get, { .s =  FIELD_ADDRESS(gentity_t, movedir) } },
  { "evaluateAcceleration", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, evaluateAcceleration) } },
  { "oldVelocity", "", TYPE_OBJECT, entity_object_vec3_set, entity_object_vec3_get, { .s =  FIELD_ADDRESS(gentity_t, oldVelocity) } },
  { "acceleration", "", TYPE_OBJECT, entity_object_vec3_set, entity_object_vec3_get, { .s =  FIELD_ADDRESS(gentity_t, acceleration) } },
  { "oldAccel", "", TYPE_OBJECT, entity_object_vec3_set, entity_object_vec3_get, { .s =  FIELD_ADDRESS(gentity_t, oldAccel) } },
  { "jerk", "", TYPE_OBJECT, entity_object_vec3_set, entity_object_vec3_get, { .s =  FIELD_ADDRESS(gentity_t, jerk) } },
  { "nextthink", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, nextthink) } },
  { "pain_debounce_time", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, pain_debounce_time) } },
  { "last_move_time", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, last_move_time) } },
  { "health", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, health) } },
  { "lastHealth", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, lastHealth) } },
  { "takedamage", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, takedamage) } },
  { "damage", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, damage) } },
  { "splashDamage", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, splashDamage) } },
  { "splashRadius", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, splashRadius) } },
  { "methodOfDeath", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, methodOfDeath) } },
  { "count", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, count) } },
  { "chain", "", TYPE_OBJECT, entity_object_entity_set, entity_object_entity_get, { .s =  FIELD_ADDRESS(gentity_t, chain) } },
  { "enemy", "", TYPE_OBJECT, entity_object_entity_set, entity_object_entity_get, { .s =  FIELD_ADDRESS(gentity_t, enemy) } },
  { "activator", "", TYPE_OBJECT, entity_object_entity_set, entity_object_entity_get, { .s =  FIELD_ADDRESS(gentity_t, activator) } },
  { "teamchain", "", TYPE_OBJECT, entity_object_entity_set, entity_object_entity_get, { .s =  FIELD_ADDRESS(gentity_t, teamchain) } },
  { "teammaster", "", TYPE_OBJECT, entity_object_entity_set, entity_object_entity_get, { .s =  FIELD_ADDRESS(gentity_t, teammaster) } },
  { "watertype", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, watertype) } },
  { "waterlevel", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, waterlevel) } },
  { "noise_index", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, noise_index) } },
  { "wait", "", TYPE_FLOAT, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, wait) } },
  { "random", "", TYPE_FLOAT, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, random) } },
  { "clipmask", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, clipmask) } },
  { "stageTeam", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, stageTeam) } },
  { "stageStage", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, stageStage) } },
  { "buildableTeam", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, buildableTeam) } },
  { "parentNode", "", TYPE_OBJECT, entity_object_entity_set, entity_object_entity_get, { .s =  FIELD_ADDRESS(gentity_t, parentNode) } },
  { "active", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, active) } },
  { "locked", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, locked) } },
  { "powered", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, powered) } },
  { "builtBy", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, builtBy) } },
  { "overmindNode", "", TYPE_OBJECT, entity_object_entity_set, entity_object_entity_get, { .s =  FIELD_ADDRESS(gentity_t, overmindNode) } },
  { "dcc", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, dcc) } },
  { "spawned", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, spawned) } },
  { "shrunkTime", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, shrunkTime) } },
  { "buildTime", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, buildTime) } },
  { "animTime", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, animTime) } },
  { "time1000", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, time1000) } },
  { "deconstruct", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, deconstruct) } },
  { "deconstructTime", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, deconstructTime) } },
  { "overmindAttackTimer", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, overmindAttackTimer) } },
  { "overmindDyingTimer", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, overmindDyingTimer) } },
  { "overmindSpawnsTimer", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, overmindSpawnsTimer) } },
  { "nextPhysicsTime", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, nextPhysicsTime) } },
  { "clientSpawnTime", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, clientSpawnTime) } },
  { "lev1Grabbed", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, lev1Grabbed) } },
  { "lev1GrabTime", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, lev1GrabTime) } },
  /* TODO !!! { "credits", "", TYPE_ARRAY, NULL, entity_credits_get, { .s = FIELD_ADDRESS(gentity_t, clipmask) } }, */
  /* TODO !!! { "creditsHash", "", TYPE_ARRAY, NULL, entity_credits_get, { .s = FIELD_ADDRESS(gentity_t, clipmask) } }, */
  { "killedBy", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, killedBy) } },
  { "targeted", "", TYPE_OBJECT, entity_object_entity_set, entity_object_entity_get, { .s =  FIELD_ADDRESS(gentity_t, targeted) } },
  { "turretAim", "", TYPE_OBJECT, entity_object_vec3_set, entity_object_vec3_get, { .s =  FIELD_ADDRESS(gentity_t, turretAim) } },
  { "turretAimRate", "", TYPE_OBJECT, entity_object_vec3_set, entity_object_vec3_get, { .s =  FIELD_ADDRESS(gentity_t, turretAimRate) } },
  { "turretSpinupTime", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, turretSpinupTime) } },
  { "animation", "", TYPE_OBJECT, entity_object_vec4_set, entity_object_vec4_get, { .s =  FIELD_ADDRESS(gentity_t, animation) } },
  { "builder", "", TYPE_OBJECT, entity_object_entity_set, entity_object_entity_get, { .s =  FIELD_ADDRESS(gentity_t, builder) } },
  { "nonSegModel", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, nonSegModel) } },
  /* TODO !!! { "bTriggers", "", TYPE_ARRAY, entity_object_set, entity_object_get, { .s =  ENTITY_BUILDER },
  { "cTriggers", "", TYPE_ARRAY, entity_object_set, entity_object_get, { .s =  ENTITY_BUILDER },
  { "wTriggers", "", TYPE_ARRAY, entity_object_set, entity_object_get, { .s =  ENTITY_BUILDER },
  { "uTriggers", "", TYPE_ARRAY, entity_object_set, entity_object_get, { .s =  ENTITY_BUILDER },*/
  { "triggerGravity", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, triggerGravity) } },
  { "suicideTime", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, suicideTime) } },
  { "lastDamageTime", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, lastDamageTime) } },
  { "lastRegenTime", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, lastRegenTime) } },
  { "zapping", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, zapping) } },
  { "wasZapping", "", TYPE_INTEGER, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, wasZapping) } },
  /* TODO !!! { "zapTargets", "", TYPE_ARRAY, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, zapTargets) } }, */
  { "zapDmg", "", TYPE_FLOAT, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, zapDmg) } },
  { "ownerClear", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, ownerClear) } },
  { "pointAgainstWorld", "", TYPE_BOOLEAN, entity_field_set, entity_field_get, { .s = FIELD_ADDRESS(gentity_t, pointAgainstWorld) } },
  { "_", "", TYPE_ANY, entity_metaset, entity_metaget, { .v = NULL } },
  { "" },
};

static scLibObjectMethod_t entity_objectMethods[] = {
  { "link", "", entity_objectMethod, { TYPE_UNDEF }, TYPE_UNDEF, { .n = ENTITY_LINK } },
  { "" },
};

static scLibObjectMethod_t entity_classMethods[] = {
  { "find", "", entity_classMethod, { TYPE_FUNCTION, TYPE_UNDEF }, TYPE_ARRAY, { .n = ENTITY_FIND } },
  { "" },
};

static scLibObjectDef_t entity_def = { 
  "Entity", "",
  0, { TYPE_UNDEF }, // an entity can't be made alone
  entity_destructor,
  entity_members, 
  entity_objectMethods, 
  NULL, // objectFields
  NULL, // classMembers
  entity_classMethods, // classMethods
  { .v = NULL }
};


/*
======================================================================

Client

======================================================================
*/

scClass_t *client_class;

struct client_struct_s
{
  gclient_t         *client;
  scDataTypeHash_t  *userdata;
};

enum client_closure_e
{
  // methods
  CLIENT_SPAWN,

  // fields
  CLIENT_OLDORIGIN,
  CLIENT_DAMAGEFROM,
  CLIENT_HOVEL,
  CLIENT_LASTPOISONCLIENT,
  CLIENT_HOVELORIGIN,
};

gclient_t *G_ClientFromScript(scObject_t *object)
{
  struct client_struct_s *cs;
  cs = object->data.data.userdata;

  return cs->client;
}

scObject_t *G_GetScriptingClient(gclient_t *client)
{
  struct client_struct_s *cs;

  // If there is no scripting entity for client, create it
  if(!client->scriptingClient)
  {
    client->scriptingClient = SC_ObjectNew(client_class);
    client->scriptingClient->data.type = TYPE_USERDATA;
    cs = BG_Alloc(sizeof(struct client_struct_s));
    client->scriptingClient->data.data.userdata = cs;

    cs->client = client;
    cs->userdata = SC_HashNew();
    SC_HashGCInc(cs->userdata);

    // One object reference is saved to client struct, so increase GC
    SC_ObjectGCInc(client->scriptingClient);
  }

  return client->scriptingClient;
}

static int client_set(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  size_t addr = closure.s;
  gclient_t *client = G_ClientFromScript(object);

  switch(in[1].type)
  {
    case TYPE_BOOLEAN:
      *(qboolean*)((void*)client + addr) = in[1].data.boolean;
      break;

    case TYPE_INTEGER:
      *(int*)((void*)client + addr) = in[1].data.integer;
      break;

    case TYPE_FLOAT:
      *(float*)((void*)client + addr) = in[1].data.floating;
      break;

    case TYPE_STRING:
      // FIXME: /!\ Memory leak /!\ */
      SC_StringGCInc(in[1].data.string);
      *(const char**)((void*)client + addr) = SC_StringToChar(in[1].data.string);
      break;

    default:
      SC_EXEC_ERROR(va("Internal error: unknow case in `client_set', %s (%d)\n", __FILE__, __LINE__));
  }

  out[0].type = TYPE_UNDEF;
  return 0;
}

static int client_get(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  size_t addr = closure.s;
  gclient_t *client = G_ClientFromScript(object);

  switch(out[0].type)
  {
    case TYPE_BOOLEAN:
      out[0].type = TYPE_BOOLEAN;
      out[0].data.boolean = *(qboolean*)((void*)client + addr);
      break;

    case TYPE_INTEGER:
      out[0].type = TYPE_INTEGER;
      out[0].data.integer = *(int*)((void*)client + addr);
      break;

    case TYPE_FLOAT:
      out[0].type = TYPE_FLOAT;
      out[0].data.floating = *(float*)((void*)client + addr);
      break;

    case TYPE_STRING:
      out[0].type = TYPE_STRING;
      out[0].data.string = SC_StringNewFromChar(*(char**)((void*)client + addr));
      break;

    default:
      SC_EXEC_ERROR(va("Internal error: unknow case in `client_set', %s (%d)\n", __FILE__, __LINE__));
  }

  out[1].type = TYPE_UNDEF;
  return 0;
}

static int client_obj_set(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  int type = closure.n;
  gclient_t *client = G_ClientFromScript(object);

  switch(type)
  {
    case CLIENT_OLDORIGIN:
      memcpy(client->oldOrigin, (vec3_t*) in[1].data.object->data.data.userdata, sizeof(vec3_t));
      break;

    case CLIENT_DAMAGEFROM:
      memcpy(client->damage_from, (vec3_t*) in[1].data.object->data.data.userdata, sizeof(vec3_t));
      break;

    case CLIENT_HOVEL:
      client->hovel = (gentity_t*) in[1].data.object->data.data.userdata;
      break;

    case CLIENT_LASTPOISONCLIENT:
      client->lastPoisonClient = (gentity_t*) in[1].data.object->data.data.userdata;
      break;

    case CLIENT_HOVELORIGIN:
      memcpy(client->hovelOrigin, (vec3_t*) in[1].data.object->data.data.userdata, sizeof(vec3_t));
      break;

    default:
      SC_EXEC_ERROR(va("Internal error: unknow case in `client_object_set', %s (%d)\n", __FILE__, __LINE__));
  }

  out[0].type = TYPE_UNDEF;
  return 0;
}

static int client_obj_get(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  int type = closure.n;
  gclient_t *client = G_ClientFromScript(object);

  switch(type)
  {
    case CLIENT_OLDORIGIN:
      out[0].type = TYPE_OBJECT;
      out[0].data.object = SC_Vec3FromVec3_t(client->oldOrigin);
      break;

    case CLIENT_DAMAGEFROM:
      out[0].type = TYPE_OBJECT;
      out[0].data.object = SC_Vec3FromVec3_t(client->damage_from);
      break;

    case CLIENT_HOVEL:
      out[0].type = TYPE_OBJECT;
      out[0].data.object = G_GetScriptingEntity(client->hovel);
      break;

    case CLIENT_LASTPOISONCLIENT:
      out[0].type = TYPE_OBJECT;
      out[0].data.object = G_GetScriptingEntity(client->lastPoisonClient);
      break;

    case CLIENT_HOVELORIGIN:
      out[0].type = TYPE_OBJECT;
      out[0].data.object = SC_Vec3FromVec3_t(client->hovelOrigin);
      break;

    default:
      SC_EXEC_ERROR(va("Internal error: unknow case in `client_object_get', %s (%d)\n", __FILE__, __LINE__));
  }

  out[1].type = TYPE_UNDEF;
  return 0;
}

static int client_metaset(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  struct client_struct_s *cs = object->data.data.userdata;

  SC_HashSet(cs->userdata, SC_StringToChar(in[1].data.string), &in[2]);

  out[0].type = TYPE_UNDEF;
  return 0;
}

static int client_metaget(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  struct client_struct_s *cs = object->data.data.userdata;

  SC_HashSet(cs->userdata, SC_StringToChar(in[1].data.string), &out[0]);

  out[1].type = TYPE_UNDEF;
  return 0;
}

static int client_vec3_set(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  int addr = closure.n;
  gclient_t *client = G_ClientFromScript(object);

  memcpy(*(vec3_t**)((void*)client + addr), (vec3_t*) in[1].data.object->data.data.userdata, sizeof(vec3_t));

  return 0;
}

static int client_vec3_get(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  int addr = closure.n;
  gclient_t *client = G_ClientFromScript(object);

  out[0].type = TYPE_OBJECT;
  out[0].data.object = SC_Vec3FromVec3_t(*(float**)((void*)client + addr));

  return 0;
}

static int client_method(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  gclient_t *client =  G_ClientFromScript(object);
  int type = closure.n;

  switch(type)
  {
    case CLIENT_SPAWN:
      {
        gentity_t *ent, *spawn;
        vec3_t origin;

        ent = g_entities + client->ps.clientNum;

        if(in[1].type == TYPE_UNDEF)
        {
          ClientSpawn(ent, NULL, NULL, NULL);
        }
        else
        {
          spawn = G_EntityFromScript(in[1].data.object);
          G_CheckSpawnPoint(spawn->s.number, spawn->s.origin, spawn->s.origin2, client->pers.teamSelection, origin);
          ClientSpawn(ent, spawn, origin, spawn->s.angles);
        }
      }
      break;

    default:
      SC_EXEC_ERROR(va("Internal error: unknow case in `client_method', %s (%d)\n", __FILE__, __LINE__));
  }

  return 0;
}

static int client_destructor(scDataTypeValue_t *in, scDataTypeValue_t *out, scClosure_t closure)
{
  scObject_t *object = in[0].data.object;
  struct client_struct_s *cs = object->data.data.userdata;

  SC_HashGCDec(cs->userdata);

  out[0].type = TYPE_UNDEF;
  return 0;
}

static scLibObjectMember_t client_members[] = {
  // Player state
  { "commandTime", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.commandTime) } },
  { "pm_type", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.pm_type) } },
  { "bobCycle", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.bobCycle) } },
  { "pm_flags", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.pm_flags) } },
  { "pm_time", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.pm_time) } },
  { "origin", "", TYPE_OBJECT, client_vec3_set, client_vec3_get, { .s = FIELD_ADDRESS(gclient_t, ps.origin) } },
  { "velocity", "", TYPE_OBJECT, client_vec3_set, client_vec3_get, { .s = FIELD_ADDRESS(gclient_t, ps.velocity) } },
  { "weaponTime", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.weaponTime) } },
  { "gravity", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.gravity) } },
  { "speed", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.speed) } },
  /*{ "delta_angles", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.delta_angles) } },*/
  { "groundEntityNum", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.groundEntityNum) } },
  { "legsTimer", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.legsTimer) } },
  { "legsAnim", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.legsAnim) } },
  { "torsoTimer", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.torsoTimer) } },
  { "torsoAnim", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.torsoAnim) } },
  { "movementDir", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.movementDir) } },
  { "grapplePoint", "", TYPE_OBJECT, client_vec3_set, client_vec3_get, { .s = FIELD_ADDRESS(gclient_t, ps.grapplePoint) } },
  { "eFlags", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.eFlags) } },
  { "eventSequence", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.eventSequence) } },
  { "events", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.events) } },
  { "eventParms", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.eventParms) } },
  { "externalEventTime", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.externalEventTime) } },
  { "clientNum", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.clientNum) } },
  { "weapon", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.weapon) } },
  { "weaponstate", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.weaponstate) } },
  { "viewangles", "", TYPE_OBJECT, client_vec3_set, client_vec3_get, { .s = FIELD_ADDRESS(gclient_t, ps.viewangles) } },
  { "damageEvent", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.damageEvent) } },
  { "damageYaw", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.damageYaw) } },
  { "damagePitch", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.damagePitch) } },
  { "damageCount", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.damageCount) } },
  { "stat_health", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.stats[STAT_HEALTH]) } },
  { "stat_items", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.stats[STAT_ITEMS]) } },
  { "stat_slots", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.stats[STAT_SLOTS]) } },
  { "stat_activeitems", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.stats[STAT_ACTIVEITEMS]) } },
  { "stat_weapons", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.stats[STAT_WEAPONS]) } },
  { "stat_weapons2", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.stats[STAT_WEAPONS2]) } },
  { "stat_max_health", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.stats[STAT_MAX_HEALTH]) } },
  { "stat_class", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.stats[STAT_CLASS]) } },
  { "stat_team", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.stats[STAT_TEAM]) } },
  { "stat_state", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.stats[STAT_STATE]) } },
  { "stat_misc", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.stats[STAT_MISC]) } },
  { "stat_buildable", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.stats[STAT_BUILDABLE]) } },
  { "stat_falldist", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.stats[STAT_FALLDIST]) } },
  { "stat_viewlock", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.stats[STAT_VIEWLOCK]) } },
  { "pers_score", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.persistant[PERS_SCORE]) } },
  { "pers_hits", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.persistant[PERS_HITS]) } },
  { "pers_spawns", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.persistant[PERS_SPAWNS]) } },
  { "pers_specstate", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.persistant[PERS_SPECSTATE]) } },
  { "pers_spawn_count", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.persistant[PERS_SPAWN_COUNT]) } },
  { "pers_attacker", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.persistant[PERS_ATTACKER]) } },
  { "pers_killed", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.persistant[PERS_KILLED]) } },
  { "pers_state", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.persistant[PERS_STATE]) } },
  { "pers_credit", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.persistant[PERS_CREDIT]) } },
  { "pers_queuepos", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.persistant[PERS_QUEUEPOS]) } },
  { "pers_newweapon", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.persistant[PERS_NEWWEAPON]) } },
  /*{ "misc...", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.misc...) } },*/
  { "ammo", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.ammo) } },
  { "clips", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.clips) } },
  { "ammo_extra", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.ammo_extra) } },
  { "generic1", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.generic1) } },
  { "loopSound", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.loopSound) } },
  { "otherEntityNum", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.otherEntityNum) } },
  { "ping", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.ping) } },
  { "pmove_framecount", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.pmove_framecount) } },
  { "jumppad_frame", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.jumppad_frame) } },
  { "entityEventSequence", "", TYPE_INTEGER, client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, ps.entityEventSequence) } },
  // Session
  { "spectatorTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, sess.spectatorTime) } },
  { "spectatorState", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, sess.spectatorState) } },
  { "spectatorClient", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, sess.spectatorClient) } },
  { "wins", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, sess.wins) } },
  { "losses", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, sess.losses) } },
  { "teamLeader", "", TYPE_BOOLEAN,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, sess.teamLeader) } },
  /*{ "ignoreList", "", TYPE_BOOLEAN,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, sess.ignoreList) } },*/

  { "readyToExit", "", TYPE_BOOLEAN,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, readyToExit) } },
  { "noclip", "", TYPE_BOOLEAN,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, noclip) } },
  { "lastCmdTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lastCmdTime) } },
  { "buttons", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, buttons) } },
  { "oldbuttons", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, oldbuttons) } },
  { "latched_buttons", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, latched_buttons) } },
  { "oldOrigin", "", TYPE_OBJECT,  client_obj_set, client_obj_get, { .n = CLIENT_OLDORIGIN } },
  { "damage_armor", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, damage_armor) } },
  { "damage_blood", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, damage_blood) } },
  { "damage_knockback", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, damage_knockback) } },
  { "damage_from", "", TYPE_OBJECT,  client_obj_set, client_obj_get, { .n = CLIENT_DAMAGEFROM } },
  { "damage_fromWorld", "", TYPE_BOOLEAN,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, damage_fromWorld) } },
  { "lastkilled_client", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lastkilled_client) } },
  { "lasthurt_client", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lasthurt_client) } },
  { "lasthurt_mod", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lasthurt_mod) } },
  { "respawnTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, respawnTime) } },
  { "inactivityTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, inactivityTime) } },
  { "inactivityWarning", "", TYPE_BOOLEAN,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, inactivityWarning) } },
  { "rewardTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, rewardTime) } },
  { "boostedTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, boostedTime) } },
  { "airOutTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, airOutTime) } },
  { "lastKillTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lastKillTime) } },
  { "fireHeld", "", TYPE_BOOLEAN,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, fireHeld) } },
  { "fire2Held", "", TYPE_BOOLEAN,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, fire2Held) } },
  { "lastCmdTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lastCmdTime) } },
  /* { "hook", "", TYPE_OBJECT,  client_object_set, client_object_get, { .n = CLIENT_HOOK }, */
  { "switchTeamTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, switchTeamTime) } },
  { "time100", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, time100) } },
  { "time1000", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, time1000) } },
  { "areabits", "", TYPE_STRING,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, areabits) } },
  { "lastCmdTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lastCmdTime) } },
  { "hos.l", "", TYPE_OBJECT,  client_obj_set, client_obj_get, { .n = CLIENT_HOVEL } },
  { "lastPoisonTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lastPoisonTime) } },
  { "poisonImmunityTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, poisonImmunityTime) } },
  { "lastPoisonClient", "", TYPE_OBJECT,  client_obj_set, client_obj_get, { .n = CLIENT_LASTPOISONCLIENT } },
  { "lastPoisonCloudedTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lastPoisonCloudedTime) } },
  { "grabExpiryTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, grabExpiryTime) } },
  { "lastLockTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lastLockTime) } },
  { "lastSlowTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lastSlowTime) } },
  { "lastMedKitTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lastMedKitTime) } },
  { "medKitHealthToRestore", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, medKitHealthToRestore) } },
  { "medKitIncrementTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, medKitIncrementTime) } },
  { "lastCreepSlowTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lastCreepSlowTime) } },
  { "charging", "", TYPE_BOOLEAN,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, charging) } },
  { "hos.lOrigin", "", TYPE_OBJECT,  client_obj_set, client_obj_get, { .n = CLIENT_HOVELORIGIN } },
  { "lastFlameBall", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lastFlameBall) } },
  // Unlagged stuff useless
  { "voiceEnthusiasm", "", TYPE_FLOAT,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, voiceEnthusiasm) } },
  { "lastVoiceCmd", "", TYPE_STRING,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lastVoiceCmd) } },
  { "lcannonStartTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lcannonStartTime) } },
  { "lastCrushTime", "", TYPE_INTEGER,  client_set, client_get, { .s = FIELD_ADDRESS(gclient_t, lastCrushTime) } },
  { "_", "", TYPE_INTEGER,  client_metaset, client_metaget, { .v = NULL } },
  { "" },
};

static scLibObjectMethod_t client_methods[] = {
  { "spawn", "", client_method, { TYPE_OBJECT, TYPE_UNDEF }, TYPE_UNDEF, { .n = CLIENT_SPAWN } },
  { "" },
};

static scLibObjectDef_t client_def = { 
  "Client", "",
  0, { TYPE_UNDEF }, // a client can't be made alone
  client_destructor,
  client_members, 
  client_methods, 
  NULL, // object fields
  NULL, // class members
  NULL, // class methods
  { .v = NULL }
};


static scConstant_t constants[] = {
  { "TEAM_NONE", TYPE_INTEGER, { .n = TEAM_NONE } },
  { "TEAM_ALIENS", TYPE_INTEGER, { .n = TEAM_ALIENS } },
  { "TEAM_HUMANS", TYPE_INTEGER, { .n = TEAM_HUMANS } },

  { "SPECTATOR_NOT", TYPE_INTEGER, { .n = SPECTATOR_NOT } },
  { "SPECTATOR_FREE", TYPE_INTEGER, { .n = SPECTATOR_FREE } },
  { "SPECTATOR_LOCKED", TYPE_INTEGER, { .n = SPECTATOR_LOCKED } },
  { "SPECTATOR_FOLLOW", TYPE_INTEGER, { .n = SPECTATOR_FOLLOW } },
  { "SPECTATOR_SCOREBOARD", TYPE_INTEGER, { .n = SPECTATOR_SCOREBOARD } },

  { "ET_GENERAL", TYPE_INTEGER, { .n = ET_GENERAL } },
  { "ET_PLAYER", TYPE_INTEGER, { .n = ET_PLAYER } },
  { "ET_ITEM", TYPE_INTEGER, { .n = ET_ITEM } },
  { "ET_BUILDABLE", TYPE_INTEGER, { .n = ET_BUILDABLE } },
  { "ET_MISSILE", TYPE_INTEGER, { .n = ET_MISSILE } },
  { "ET_MOVER", TYPE_INTEGER, { .n = ET_MOVER } },
  { "ET_BEAM", TYPE_INTEGER, { .n = ET_BEAM } },
  { "ET_PORTAL", TYPE_INTEGER, { .n = ET_PORTAL } },
  { "ET_SPEAKER", TYPE_INTEGER, { .n = ET_SPEAKER } },
  { "ET_PUSH_TRIGGER", TYPE_INTEGER, { .n = ET_PUSH_TRIGGER } },
  { "ET_TELEPORT_TRIGGER", TYPE_INTEGER, { .n = ET_TELEPORT_TRIGGER } },
  { "ET_INVISIBLE", TYPE_INTEGER, { .n = ET_INVISIBLE } },
  { "ET_GRAPPLE", TYPE_INTEGER, { .n = ET_GRAPPLE } },
  { "ET_CORPSE", TYPE_INTEGER, { .n = ET_CORPSE } },
  { "ET_PARTICLE_SYSTEM", TYPE_INTEGER, { .n = ET_PARTICLE_SYSTEM } },
  { "ET_ANIMMAPOBJ", TYPE_INTEGER, { .n = ET_ANIMMAPOBJ } },
  { "ET_MODELDOOR", TYPE_INTEGER, { .n = ET_MODELDOOR } },
  { "ET_LIGHTFLARE", TYPE_INTEGER, { .n = ET_LIGHTFLARE } },
  { "ET_EVENTS", TYPE_INTEGER, { .n = ET_EVENTS } },

  { "BA_NONE", TYPE_INTEGER, { .n = BA_NONE } },
  { "BA_A_SPAWN", TYPE_INTEGER, { .n = BA_A_SPAWN } },
  { "BA_A_OVERMIND", TYPE_INTEGER, { .n = BA_A_OVERMIND } },
  { "BA_A_BARRICADE", TYPE_INTEGER, { .n = BA_A_BARRICADE } },
  { "BA_A_ACIDTUBE", TYPE_INTEGER, { .n = BA_A_ACIDTUBE } },
  { "BA_A_TRAPPER", TYPE_INTEGER, { .n = BA_A_TRAPPER } },
  { "BA_A_BOOSTER", TYPE_INTEGER, { .n = BA_A_BOOSTER } },
  { "BA_A_HIVE", TYPE_INTEGER, { .n = BA_A_HIVE } },
  { "BA_A_HOVEL", TYPE_INTEGER, { .n = BA_A_HOVEL } },
  { "BA_H_SPAWN", TYPE_INTEGER, { .n = BA_H_SPAWN } },
  { "BA_H_MGTURRET", TYPE_INTEGER, { .n = BA_H_MGTURRET } },
  { "BA_H_TESLAGEN", TYPE_INTEGER, { .n = BA_H_TESLAGEN } },
  { "BA_H_ARMOURY", TYPE_INTEGER, { .n = BA_H_ARMOURY } },
  { "BA_H_DCC", TYPE_INTEGER, { .n = BA_H_DCC } },
  { "BA_H_DCC", TYPE_INTEGER, { .n = BA_H_DCC } },
  { "BA_H_MEDISTAT", TYPE_INTEGER, { .n = BA_H_MEDISTAT } },
  { "BA_H_REACTOR", TYPE_INTEGER, { .n = BA_H_REACTOR } },
  { "BA_H_REPEATER", TYPE_INTEGER, { .n = BA_H_REPEATER } },
  { "BA_NUM_BUILDABLES", TYPE_INTEGER, { .n = BA_NUM_BUILDABLES } },

  { "" }
};

/*
================
G_InitScript

Initialize scripting system and load libraries
================
*/

static int autoload_dir(const char *dirname)
{
  int             numdirs;
  char            dirlist[1024];
  char            *dirptr;
  char            filename[128];
  char            *fnptr;
  int             i, dirlen = 0, numFiles = 0;

  strcpy(filename, dirname);
  fnptr = filename + strlen(filename);

  numdirs = trap_FS_GetFileList(dirname, "", dirlist, 1024);
  dirptr = dirlist;
  for(i = 0; i < numdirs; i++, dirptr += dirlen + 1)
  {
    dirlen = strlen(dirptr);
    strcpy(fnptr, dirptr);

    // load the file
    if (SC_RunScript(SC_LangageFromFilename(filename), filename) != -1 )
      numFiles++;
  }

  numdirs = trap_FS_GetFileList(dirname, "/", dirlist, 1024);
  dirptr = dirlist;
  for(i = 0; i < numdirs; i++, dirptr += dirlen + 1)
  {
    // ignore hidden directories
    if(dirptr[0] != '.')
    {
      strcpy(fnptr, va("%s/", dirptr));

      // load recursively
      numFiles += autoload_dir(filename);
    }
    dirlen = strlen(dirptr);
  }

  return numFiles;
}

int G_ExecFileNow(const char *filename)
{
  char *text;
  fileHandle_t h;
  int len;
  char *idx, *lidx;

  Com_Printf(va("load \"%s\"\n", filename));
  len = trap_FS_FOpenFile(filename, &h, FS_READ);
  text = BG_Alloc(len+1);

  trap_FS_Read(text, len, h);
  text[len] = '\0';
  trap_FS_FCloseFile(h);

  lidx = text;
  while((idx = strchr(lidx, '\n')))
  {
    *idx = '\0';

    Com_Printf("exec %s\n", lidx);
    trap_SendConsoleCommand(EXEC_NOW, lidx);
    lidx = idx + 1;
  }

  BG_Free(text);

  return 0;
}

void G_InitScript( void )
{
  char            buf[MAX_STRING_CHARS];
  int             numFiles;

  Com_Printf("------- Game Scripting System Initializing -------\n");
  SC_Init();

  SC_AddLibrary( "game", game_lib );
  entity_class = SC_AddClass( "game", &entity_def );
  client_class = SC_AddClass( "game", &client_def );
  SC_Constant_Add(constants);

  SC_LoadLangages();

  // Autoload directories
  Com_Printf("load global scripts:\n");
  numFiles = autoload_dir(GAME_SCRIPT_DIRECTORY "global/");
  Com_Printf("%i global files parsed\n", numFiles);

  Com_Printf("load map specific scripts:\n");
  trap_Cvar_VariableStringBuffer("mapname", buf, sizeof(buf));
  numFiles = autoload_dir(va(GAME_SCRIPT_DIRECTORY "map-%s/", buf));
  Com_Printf("%i local files parsed\n", numFiles);

  G_ExecFileNow(va(GAME_SCRIPT_DIRECTORY "map-%s.cfg", buf));
  G_ExecFileNow(GAME_SCRIPT_DIRECTORY "autoexec.cfg");

  Com_Printf("-----------------------------------\n");
}
