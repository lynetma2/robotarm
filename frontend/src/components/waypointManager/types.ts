// src/components/waypoints/types.ts

export interface Waypoint {
  id: number
  x: number
  y: number
  speed: number
}

export interface Sequence {
  id: number
  title: string
  description: string
  waypoints: Waypoint[]
}
