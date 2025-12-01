// --- TYPE DEFINITIONS ---
export interface Pose {
  x: number;
  y: number;
  z: number;
  roll: number;
  pitch: number;
  yaw: number;
}

export interface Step {
  id?: number;
  name:string;
  pose: Pose;
  interpolation: 'LINEAR' | 'JOINT';
  speed: number;
}

export interface Settings {
  loop: boolean;
}

// Represents a sequence that might be new (no ID)
export interface Sequence {
  id?: number;
  title: string;
  description: string;
  lastModified: string;
  steps: Step[];
  settings: Settings;
}

// Represents a sequence that is saved and has an ID
export interface SavedSequence extends Sequence {
  id: string;
}

// Jog event information
export enum DirectionEnum {
  clockwise = 1,
  counterClockwise = -1
}

export interface JogEvent {
  motorId: number;
  direction: DirectionEnum;
  speed: number;
}

export interface command {
  command: "jog" |
}
