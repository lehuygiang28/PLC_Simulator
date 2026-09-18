/** @noSelfInFile */
/** PLC Simulator script API — global functions provided by the Lua engine. */

declare function SetInt16(addr: string, value: number): void;
declare function SetInt32(addr: string, value: number): void;
declare function SetFloat(addr: string, value: number): void;
declare function SetDouble(addr: string, value: number): void;
declare function SetString(addr: string, value: string): void;

declare function GetInt16(addr: string): number;
declare function GetInt32(addr: string): number;
declare function GetFloat(addr: string): number;
declare function GetDouble(addr: string): number;
declare function GetString(addr: string): string;

declare function SetBit(addr: string, value: number): void;
declare function GetBit(addr: string): number;

declare function MoveAbsInt32(x: number, y: number, angle: number): void;
declare function MoveAbsFloat(x: number, y: number, angle: number): void;
declare function MoveRelativeInt32(dx: number, dy: number, dAngle: number): void;
declare function MoveRelativeFloat(dx: number, dy: number, dAngle: number): void;
declare function WriteCurrentPosInt32(x: number, y: number, angle: number): void;
declare function WriteCurrentPosFloat(x: number, y: number, angle: number): void;

declare function IsLoopValid(): boolean;
declare function sleep(ms: number): void;
