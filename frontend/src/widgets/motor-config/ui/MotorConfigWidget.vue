<script setup lang="ts">
import { ref } from 'vue'
import { Settings, Save } from 'lucide-vue-next'
import { Button } from '@/shared/ui/button'
import { MotorCard, useMotorConfig } from '@/entities/motor'

const { motorConfigs, resetMotor, hasChanges, markAsSaved } = useMotorConfig()
const expandedMotor = ref<number | null>(null)

const toggleExpanded = (id: number) => {
  expandedMotor.value = expandedMotor.value === id ? null : id
}

const handleSave = () => {
  console.log('Saving configuration:', motorConfigs.value)
  // Implement API call here
  markAsSaved()
}
</script>

<template>
  <div class="space-y-8">
    <!-- Widget Header -->
    <div class="flex items-center justify-between">
      <div class="flex items-center gap-4">
        <div class="p-3 rounded-xl bg-primary text-primary-foreground shadow-sm">
          <Settings class="w-6 h-6" />
        </div>
        <div>
          <h1 class="text-2xl font-bold tracking-tight">TMC2209 Configuration</h1>
          <p class="text-muted-foreground">Manage all 6 stepper motor drivers</p>
        </div>
      </div>

      <div class="flex items-center gap-4">
        <div v-if="hasChanges" class="flex items-center gap-2 text-amber-600 bg-amber-50 px-3 py-1 rounded-full border border-amber-200 text-xs font-medium animate-in fade-in slide-in-from-right-4">
          <span class="relative flex h-2 w-2">
            <span class="animate-ping absolute inline-flex h-full w-full rounded-full bg-amber-400 opacity-75"></span>
            <span class="relative inline-flex rounded-full h-2 w-2 bg-amber-500"></span>
          </span>
          Unsaved Changes
        </div>

        <Button class="gap-2 shadow-sm" @click="handleSave">
          <Save class="w-4 h-4" />
          Save Configuration
        </Button>
      </div>
    </div>

    <!-- Grid -->
    <div class="grid grid-cols-1 lg:grid-cols-2 gap-6 items-start">
      <MotorCard
        v-for="motor in motorConfigs"
        :key="motor.id"
        :motor="motor"
        :expanded="expandedMotor === motor.id"
        @toggle="toggleExpanded"
        @reset="resetMotor"
      />
    </div>
  </div>
</template>