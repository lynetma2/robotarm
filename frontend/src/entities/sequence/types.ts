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
  name: string;
  pose: Pose;
  interpolation: 'LINEAR' | 'JOINT';
  speed: number;
}

export interface Settings {
  loop: boolean;
}

export interface Sequence {
  id?: number;
  title: string;
  description: string;
  lastModified?: string;
  steps: Step[];
  settings: Settings;
}
