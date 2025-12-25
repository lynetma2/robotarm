export interface MotorConfig {
  id: number
  name: string
  enabled: boolean
  runCurrent: number
  holdCurrent: number
  microsteps: number
  stallThreshold: number
  coolStep: boolean
  spreadCycle: boolean
  interpolation: boolean
  pwmAutoScale: boolean
}

export interface FeatureOption {
  key: keyof Pick<MotorConfig, 'coolStep' | 'spreadCycle' | 'interpolation' | 'pwmAutoScale'>
  label: string
}