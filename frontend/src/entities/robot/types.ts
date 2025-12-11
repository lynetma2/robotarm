export interface Joint {
  id: number;
  name: string;
}

export enum JogDirection {
  POSITIVE = 1,
  NEGATIVE = -1
}

// The payload structure expected by your backend
export interface JogCommand {
  motorId: number;
  direction: JogDirection;
  speed: number; // 0.0 to 1.0
}

export interface Coordinate {
  label: string;
  value: string | number;
}
