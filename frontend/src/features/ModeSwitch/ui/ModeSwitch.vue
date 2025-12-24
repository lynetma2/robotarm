<script setup lang="ts">
import { Switch } from "@/shared/ui/switch"
import { Label } from "@/shared/ui/label"
import { ref } from "vue"

const isLive = ref(false)
</script>

<template>
  <div class="flex items-center space-x-2">
    <Switch id="mode-switch" v-model:checked="isLive" />
    <Label for="mode-switch">{{ isLive ? 'Live' : 'Simulation' }}</Label>
  </div>
</template>
