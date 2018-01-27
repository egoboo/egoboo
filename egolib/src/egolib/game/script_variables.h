#pragma once

#include "egolib/platform.h"

struct script_state_t;
struct ai_state_t;
class Object;

int32_t load_VARTMPX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTMPY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTMPDISTANCE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTMPTURN(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTMPARGUMENT(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARRAND(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFTURN(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFCOUNTER(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFORDER(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFMORALE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFLIFE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETDISTANCE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETTURN(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARLEADERX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARLEADERY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARLEADERDISTANCE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARLEADERTURN(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARGOTOX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARGOTOY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARGOTODISTANCE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETTURNTO(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARPASSAGE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARWEIGHT(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFALTITUDE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFID(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFHATEID(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFMANA(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETSTR(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETINT(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETDEX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETLIFE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETMANA(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETSPEEDX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETSPEEDY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETSPEEDZ(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFSPAWNX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFSPAWNY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFSTATE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFCONTENT(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFSTR(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFINT(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFDEX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFMANAFLOW(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETMANAFLOW(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFATTACHED(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETLEVEL(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETZ(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFINDEX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VAROWNERX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VAROWNERY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VAROWNERTURN(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VAROWNERDISTANCE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VAROWNERTURNTO(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARXYTURNTO(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFMONEY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFACCEL(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETEXP(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFAMMO(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETAMMO(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETMONEY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETTURNAWAY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFLEVEL(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETRELOADTIME(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSPAWNDISTANCE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETMAXLIFE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETTEAM(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETARMOR(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARDIFFICULTY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTIMEHOURS(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTIMEMINUTES(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTIMESECONDS(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARDATEMONTH(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARDATEDAY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSWINGTURN(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARXYDISTANCE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARSELFZ(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);

int32_t load_VARTARGETALTITUDE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader);
