export interface LogMessage {
  timestamp: string
  level: 'INFO' | 'WARN' | 'ERROR' | 'DEBUG' | 'RAW'
  message: string
}
