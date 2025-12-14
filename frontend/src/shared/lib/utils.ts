import type { ClassValue } from "clsx"
import { clsx } from "clsx"
import { twMerge } from "tailwind-merge"
import type {Coordinate, Telemetry} from "@/entities/robot/types.ts";

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs))
}

export function toCoordinates(value: Telemetry) {
  let newCoordinates: Coordinate[] = []
  let keys = ["X", "Y", "Z", "R", "P", "Y"]

  value.motor_positions.forEach((position, index) => {
    let pos = position ?? 0
    let key = keys[index] ?? "N/A"
    newCoordinates.push({label: key, value: pos})
  })
  return newCoordinates
}
