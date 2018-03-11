#pragma once

/// Special modes for particle reflections from characters
enum MissileTreatment {
	MissileTreatment_Normal = 0, ///< Treat missiles normally
	MissileTreatment_Deflect,    ///< Deflect incoming missiles
	MissileTreatment_Reflect,    ///< Reflect them back!
};
