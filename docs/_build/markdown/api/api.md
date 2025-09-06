# Gaussian Extractor API

Comprehensive API documentation for the Gaussian Extractor.

## Class Hierarchy

<ul class="treeView" id="class-treeView">
  <li>
    <ul class="collapsibleList">
      <li>Struct <a href="structCheckSummary.html#exhale-struct-structCheckSummary">CheckSummary</a></li><li>Struct <a href="structCommandContext.html#exhale-struct-structCommandContext">CommandContext</a></li><li>Struct <a href="structConfigValue.html#exhale-struct-structConfigValue">ConfigValue</a></li><li>Struct <a href="structCreateSummary.html#exhale-struct-structCreateSummary">CreateSummary</a></li><li>Struct <a href="structExtractSummary.html#exhale-struct-structExtractSummary">ExtractSummary</a></li><li>Struct <a href="structHighLevelEnergyData.html#exhale-struct-structHighLevelEnergyData">HighLevelEnergyData</a></li><li>Struct <a href="structJobCheckResult.html#exhale-struct-structJobCheckResult">JobCheckResult</a></li><li>Struct <a href="structJobResources.html#exhale-struct-structJobResources">JobResources</a></li><li>Struct <a href="structProcessingContext.html#exhale-struct-structProcessingContext">ProcessingContext</a></li><li>Struct <a href="structResult.html#exhale-struct-structResult">Result</a></li><li>Class <a href="classCommandParser.html#exhale-class-classCommandParser">CommandParser</a></li><li>Class <a href="classConfigManager.html#exhale-class-classConfigManager">ConfigManager</a></li><li>Class <a href="classCoordExtractor.html#exhale-class-classCoordExtractor">CoordExtractor</a></li><li>Class <a href="classCreateInput.html#exhale-class-classCreateInput">CreateInput</a></li><li>Class <a href="classFileHandleManager.html#exhale-class-classFileHandleManager">FileHandleManager</a><ul><li class="lastChild">Class <a href="classFileHandleManager_1_1FileGuard.html#exhale-class-classFileHandleManager-1-1FileGuard">FileHandleManager::FileGuard</a></li></ul></li><li>Class <a href="classHighLevelEnergyCalculator.html#exhale-class-classHighLevelEnergyCalculator">HighLevelEnergyCalculator</a></li><li>Class <a href="classJobChecker.html#exhale-class-classJobChecker">JobChecker</a></li><li>Class <a href="classJobSchedulerDetector.html#exhale-class-classJobSchedulerDetector">JobSchedulerDetector</a></li><li>Class <a href="classMemoryMonitor.html#exhale-class-classMemoryMonitor">MemoryMonitor</a></li><li>Class <a href="classParameterParser.html#exhale-class-classParameterParser">ParameterParser</a></li><li>Class <a href="classThreadPool.html#exhale-class-classThreadPool">ThreadPool</a></li><li>Class <a href="classThreadSafeErrorCollector.html#exhale-class-classThreadSafeErrorCollector">ThreadSafeErrorCollector</a></li><li>Enum <a href="enum_create_\_input_8h_1ae2fb950fc8d1f9388248733404704d2f.html#exhale-enum-create-input-8h-1ae2fb950fc8d1f9388248733404704d2f">CalculationType</a></li><li>Enum <a href="enum_command_\_system_8h_1a21e038f5b8958e203d28bc4f18472352.html#exhale-enum-command-system-8h-1a21e038f5b8958e203d28bc4f18472352">CommandType</a></li><li>Enum <a href="enum_config_\_manager_8h_1a32f23b552ade08d96d49cc3061a27165.html#exhale-enum-config-manager-8h-1a32f23b552ade08d96d49cc3061a27165">ConfigCategory</a></li><li>Enum <a href="enum_utils_8h_1a0dc5721c2ff6b50758dc4f9688ea3cfe.html#exhale-enum-utils-8h-1a0dc5721c2ff6b50758dc4f9688ea3cfe">FileReadMode</a></li><li>Enum <a href="enum_job_\_checker_8h_1a5df92147ec0babfdf2aa8856833bb4b8.html#exhale-enum-job-checker-8h-1a5df92147ec0babfdf2aa8856833bb4b8">JobStatus</a></li><li class="lastChild">Enum <a href="enum_job_\_scheduler_8h_1a342cd0e47545da3e679b393fc548c6b1.html#exhale-enum-job-scheduler-8h-1a342cd0e47545da3e679b393fc548c6b1">SchedulerType</a></li>
    </ul>
  </li><!-- only tree view element -->
</ul><!-- /treeView class-treeView -->
<!-- end raw html for treeView -->

## File Hierarchy

<ul class="treeView" id="file-treeView">
  <li>
    <ul class="collapsibleList">
      <li class="lastChild">Directory <a href="dir_core.html#dir-core">core</a><ul><li>File <a href="file_core_command_system.h.html#file-core-command-system.h">command_system.h</a></li><li>File <a href="file_core_config_manager.h.html#file-core-config-manager.h">config_manager.h</a></li><li>File <a href="file_core_coord_extractor.h.html#file-core-coord-extractor.h">coord_extractor.h</a></li><li>File <a href="file_core_create_input.h.html#file-core-create-input.h">create_input.h</a></li><li>File <a href="file_core_gaussian_extractor.h.html#file-core-gaussian-extractor.h">gaussian_extractor.h</a></li><li>File <a href="file_core_help_utils.h.html#file-core-help-utils.h">help_utils.h</a></li><li>File <a href="file_core_high_level_energy.h.html#file-core-high-level-energy.h">high_level_energy.h</a></li><li>File <a href="file_core_interactive_mode.h.html#file-core-interactive-mode.h">interactive_mode.h</a></li><li>File <a href="file_core_job_checker.h.html#file-core-job-checker.h">job_checker.h</a></li><li>File <a href="file_core_job_scheduler.h.html#file-core-job-scheduler.h">job_scheduler.h</a></li><li>File <a href="file_core_metadata.h.html#file-core-metadata.h">metadata.h</a></li><li>File <a href="file_core_module_executor.h.html#file-core-module-executor.h">module_executor.h</a></li><li>File <a href="file_core_parameter_parser.h.html#file-core-parameter-parser.h">parameter_parser.h</a></li><li>File <a href="file_core_utils.h.html#file-core-utils.h">utils.h</a></li><li class="lastChild">File <a href="file_core_version.h.html#file-core-version.h">version.h</a></li></ul></li>
    </ul>
  </li><!-- only tree view element -->
</ul><!-- /treeView file-treeView -->
<!-- end raw html for treeView -->

## Full API

### Namespaces

* [Namespace ConfigUtils](namespace_ConfigUtils.md)
  * [Detailed Description](namespace_ConfigUtils.md#detailed-description)
  * [Functions](namespace_ConfigUtils.md#functions)

* [Namespace FileUtils](namespace_FileUtils.md)

* [Namespace GaussianExtractor](namespace_GaussianExtractor.md)
  * [Functions](namespace_GaussianExtractor.md#functions)

* [Namespace HelpUtils](namespace_HelpUtils.md)
  * [Functions](namespace_HelpUtils.md#functions)

* [Namespace HighLevelEnergyUtils](namespace_HighLevelEnergyUtils.md)
  * [Detailed Description](namespace_HighLevelEnergyUtils.md#detailed-description)
  * [Functions](namespace_HighLevelEnergyUtils.md#functions)

* [Namespace JobCheckerUtils](namespace_JobCheckerUtils.md)
  * [Detailed Description](namespace_JobCheckerUtils.md#detailed-description)
  * [Functions](namespace_JobCheckerUtils.md#functions)

* [Namespace Metadata](namespace_Metadata.md)
  * [Functions](namespace_Metadata.md#functions)

* [Namespace Utils](namespace_Utils.md)
  * [Functions](namespace_Utils.md#functions)

### Classes and Structs

* [Struct CheckSummary](structCheckSummary.md)
  * [Struct Documentation](structCheckSummary.md#struct-documentation)
    * [`CheckSummary`](structCheckSummary.md#_CPPv412CheckSummary)
      * [`CheckSummary()`](structCheckSummary.md#_CPPv4N12CheckSummary12CheckSummaryEv)
      * [`total_files`](structCheckSummary.md#_CPPv4N12CheckSummary11total_filesE)
      * [`processed_files`](structCheckSummary.md#_CPPv4N12CheckSummary15processed_filesE)
      * [`matched_files`](structCheckSummary.md#_CPPv4N12CheckSummary13matched_filesE)
      * [`moved_files`](structCheckSummary.md#_CPPv4N12CheckSummary11moved_filesE)
      * [`failed_moves`](structCheckSummary.md#_CPPv4N12CheckSummary12failed_movesE)
      * [`errors`](structCheckSummary.md#_CPPv4N12CheckSummary6errorsE)
      * [`execution_time`](structCheckSummary.md#_CPPv4N12CheckSummary14execution_timeE)

* [Struct CommandContext](structCommandContext.md)
  * [Struct Documentation](structCommandContext.md#struct-documentation)
    * [`CommandContext`](structCommandContext.md#_CPPv414CommandContext)

* [Struct ConfigValue](structConfigValue.md)
  * [Struct Documentation](structConfigValue.md#struct-documentation)
    * [`ConfigValue`](structConfigValue.md#_CPPv411ConfigValue)
      * [`ConfigValue()`](structConfigValue.md#_CPPv4N11ConfigValue11ConfigValueEv)
      * [`ConfigValue()`](structConfigValue.md#_CPPv4N11ConfigValue11ConfigValueERKNSt6stringERKNSt6stringERKNSt6stringE)
      * [`value`](structConfigValue.md#_CPPv4N11ConfigValue5valueE)
      * [`default_value`](structConfigValue.md#_CPPv4N11ConfigValue13default_valueE)
      * [`description`](structConfigValue.md#_CPPv4N11ConfigValue11descriptionE)
      * [`category`](structConfigValue.md#_CPPv4N11ConfigValue8categoryE)
      * [`user_set`](structConfigValue.md#_CPPv4N11ConfigValue8user_setE)

* [Struct CreateSummary](structCreateSummary.md)
  * [Struct Documentation](structCreateSummary.md#struct-documentation)
    * [`CreateSummary`](structCreateSummary.md#_CPPv413CreateSummary)
      * [`CreateSummary()`](structCreateSummary.md#_CPPv4N13CreateSummary13CreateSummaryEv)
      * [`total_files`](structCreateSummary.md#_CPPv4N13CreateSummary11total_filesE)
      * [`processed_files`](structCreateSummary.md#_CPPv4N13CreateSummary15processed_filesE)
      * [`created_files`](structCreateSummary.md#_CPPv4N13CreateSummary13created_filesE)
      * [`failed_files`](structCreateSummary.md#_CPPv4N13CreateSummary12failed_filesE)
      * [`skipped_files`](structCreateSummary.md#_CPPv4N13CreateSummary13skipped_filesE)
      * [`errors`](structCreateSummary.md#_CPPv4N13CreateSummary6errorsE)
      * [`execution_time`](structCreateSummary.md#_CPPv4N13CreateSummary14execution_timeE)

* [Struct ExtractSummary](structExtractSummary.md)
  * [Struct Documentation](structExtractSummary.md#struct-documentation)
    * [`ExtractSummary`](structExtractSummary.md#_CPPv414ExtractSummary)
      * [`ExtractSummary()`](structExtractSummary.md#_CPPv4N14ExtractSummary14ExtractSummaryEv)
      * [`total_files`](structExtractSummary.md#_CPPv4N14ExtractSummary11total_filesE)
      * [`processed_files`](structExtractSummary.md#_CPPv4N14ExtractSummary15processed_filesE)
      * [`extracted_files`](structExtractSummary.md#_CPPv4N14ExtractSummary15extracted_filesE)
      * [`failed_files`](structExtractSummary.md#_CPPv4N14ExtractSummary12failed_filesE)
      * [`moved_to_final`](structExtractSummary.md#_CPPv4N14ExtractSummary14moved_to_finalE)
      * [`moved_to_running`](structExtractSummary.md#_CPPv4N14ExtractSummary16moved_to_runningE)
      * [`errors`](structExtractSummary.md#_CPPv4N14ExtractSummary6errorsE)
      * [`execution_time`](structExtractSummary.md#_CPPv4N14ExtractSummary14execution_timeE)

* [Struct HighLevelEnergyData](structHighLevelEnergyData.md)
  * [Struct Documentation](structHighLevelEnergyData.md#struct-documentation)
    * [`HighLevelEnergyData`](structHighLevelEnergyData.md#_CPPv419HighLevelEnergyData)
      * [`HighLevelEnergyData()`](structHighLevelEnergyData.md#_CPPv4N19HighLevelEnergyData19HighLevelEnergyDataEv)
      * [`HighLevelEnergyData()`](structHighLevelEnergyData.md#_CPPv4N19HighLevelEnergyData19HighLevelEnergyDataERKNSt6stringE)
      * [`filename`](structHighLevelEnergyData.md#_CPPv4N19HighLevelEnergyData8filenameE)

* [Struct JobCheckResult](structJobCheckResult.md)
  * [Struct Documentation](structJobCheckResult.md#struct-documentation)
    * [`JobCheckResult`](structJobCheckResult.md#_CPPv414JobCheckResult)
      * [`JobCheckResult()`](structJobCheckResult.md#_CPPv4N14JobCheckResult14JobCheckResultEv)
      * [`JobCheckResult()`](structJobCheckResult.md#_CPPv4N14JobCheckResult14JobCheckResultERKNSt6stringE9JobStatus)
      * [`filename`](structJobCheckResult.md#_CPPv4N14JobCheckResult8filenameE)
      * [`status`](structJobCheckResult.md#_CPPv4N14JobCheckResult6statusE)
      * [`error_message`](structJobCheckResult.md#_CPPv4N14JobCheckResult13error_messageE)
      * [`related_files`](structJobCheckResult.md#_CPPv4N14JobCheckResult13related_filesE)

* [Struct JobResources](structJobResources.md)
  * [Struct Documentation](structJobResources.md#struct-documentation)
    * [`JobResources`](structJobResources.md#_CPPv412JobResources)
      * [`scheduler_type`](structJobResources.md#_CPPv4N12JobResources14scheduler_typeE)
      * [`job_id`](structJobResources.md#_CPPv4N12JobResources6job_idE)
      * [`allocated_cpus`](structJobResources.md#_CPPv4N12JobResources14allocated_cpusE)
      * [`allocated_memory_mb`](structJobResources.md#_CPPv4N12JobResources19allocated_memory_mbE)
      * [`nodes`](structJobResources.md#_CPPv4N12JobResources5nodesE)
      * [`tasks_per_node`](structJobResources.md#_CPPv4N12JobResources14tasks_per_nodeE)
      * [`has_cpu_limit`](structJobResources.md#_CPPv4N12JobResources13has_cpu_limitE)
      * [`has_memory_limit`](structJobResources.md#_CPPv4N12JobResources16has_memory_limitE)
      * [`partition`](structJobResources.md#_CPPv4N12JobResources9partitionE)
      * [`account`](structJobResources.md#_CPPv4N12JobResources7accountE)

* [Struct ProcessingContext](structProcessingContext.md)
  * [Struct Documentation](structProcessingContext.md#struct-documentation)
    * [`ProcessingContext`](structProcessingContext.md#_CPPv417ProcessingContext)
      * [`ProcessingContext()`](structProcessingContext.md#_CPPv4N17ProcessingContext17ProcessingContextEdibjRKNSt6stringE6size_tRK12JobResources)
      * [`memory_monitor`](structProcessingContext.md#_CPPv4N17ProcessingContext14memory_monitorE)
      * [`file_manager`](structProcessingContext.md#_CPPv4N17ProcessingContext12file_managerE)
      * [`error_collector`](structProcessingContext.md#_CPPv4N17ProcessingContext15error_collectorE)
      * [`base_temp`](structProcessingContext.md#_CPPv4N17ProcessingContext9base_tempE)
      * [`concentration`](structProcessingContext.md#_CPPv4N17ProcessingContext13concentrationE)
      * [`use_input_temp`](structProcessingContext.md#_CPPv4N17ProcessingContext14use_input_tempE)
      * [`extension`](structProcessingContext.md#_CPPv4N17ProcessingContext9extensionE)
      * [`requested_threads`](structProcessingContext.md#_CPPv4N17ProcessingContext17requested_threadsE)
      * [`max_file_size_mb`](structProcessingContext.md#_CPPv4N17ProcessingContext16max_file_size_mbE)
      * [`job_resources`](structProcessingContext.md#_CPPv4N17ProcessingContext13job_resourcesE)

* [Struct Result](structResult.md)
  * [Struct Documentation](structResult.md#struct-documentation)
    * [`Result`](structResult.md#_CPPv46Result)
      * [`file_name`](structResult.md#_CPPv4N6Result9file_nameE)
      * [`etgkj`](structResult.md#_CPPv4N6Result5etgkjE)
      * [`lf`](structResult.md#_CPPv4N6Result2lfE)
      * [`GibbsFreeHartree`](structResult.md#_CPPv4N6Result16GibbsFreeHartreeE)
      * [`nucleare`](structResult.md#_CPPv4N6Result8nucleareE)
      * [`scf`](structResult.md#_CPPv4N6Result3scfE)
      * [`zpe`](structResult.md#_CPPv4N6Result3zpeE)
      * [`status`](structResult.md#_CPPv4N6Result6statusE)
      * [`phaseCorr`](structResult.md#_CPPv4N6Result9phaseCorrE)
      * [`copyright_count`](structResult.md#_CPPv4N6Result15copyright_countE)

* [Class CommandParser](classCommandParser.md)
  * [Class Documentation](classCommandParser.md#class-documentation)
    * [`CommandParser`](classCommandParser.md#_CPPv413CommandParser)

* [Class ConfigManager](classConfigManager.md)
  * [Class Documentation](classConfigManager.md#class-documentation)
    * [`ConfigManager`](classConfigManager.md#_CPPv413ConfigManager)
      * [`ConfigManager()`](classConfigManager.md#_CPPv4N13ConfigManager13ConfigManagerEv)

* [Class CoordExtractor](classCoordExtractor.md)
  * [Class Documentation](classCoordExtractor.md#class-documentation)
    * [`CoordExtractor`](classCoordExtractor.md#_CPPv414CoordExtractor)
      * [`CoordExtractor()`](classCoordExtractor.md#_CPPv4N14CoordExtractor14CoordExtractorENSt10shared_ptrI17ProcessingContextEEb)
      * [`extract_coordinates()`](classCoordExtractor.md#_CPPv4N14CoordExtractor19extract_coordinatesERKNSt6vectorINSt6stringEEE)
      * [`print_summary()`](classCoordExtractor.md#_CPPv4N14CoordExtractor13print_summaryERK14ExtractSummaryRKNSt6stringE)

* [Class CreateInput](classCreateInput.md)
  * [Class Documentation](classCreateInput.md#class-documentation)
    * [`CreateInput`](classCreateInput.md#_CPPv411CreateInput)
      * [`CreateInput()`](classCreateInput.md#_CPPv4N11CreateInput11CreateInputENSt10shared_ptrI17ProcessingContextEEb)
      * [`CreateInput()`](classCreateInput.md#_CPPv4N11CreateInput11CreateInputENSt10shared_ptrI17ProcessingContextEERKNSt6stringEb)
      * [`loadParameters()`](classCreateInput.md#_CPPv4N11CreateInput14loadParametersERKNSt6stringE)
      * [`generate_checkpoint_section()`](classCreateInput.md#_CPPv4N11CreateInput27generate_checkpoint_sectionERKNSt6stringE)
      * [`generate_title()`](classCreateInput.md#_CPPv4N11CreateInput14generate_titleEv)
      * [`select_basis_for_calculation()`](classCreateInput.md#_CPPv4NK11CreateInput28select_basis_for_calculationEv)
      * [`is_gen_basis()`](classCreateInput.md#_CPPv4NK11CreateInput12is_gen_basisERKNSt6stringE)
      * [`validate_gen_basis_requirements()`](classCreateInput.md#_CPPv4NK11CreateInput31validate_gen_basis_requirementsEv)
      * [`create_inputs()`](classCreateInput.md#_CPPv4N11CreateInput13create_inputsERKNSt6vectorINSt6stringEEE)
      * [`set_calculation_type()`](classCreateInput.md#_CPPv4N11CreateInput20set_calculation_typeE15CalculationType)
      * [`set_functional()`](classCreateInput.md#_CPPv4N11CreateInput14set_functionalERKNSt6stringE)
      * [`set_basis()`](classCreateInput.md#_CPPv4N11CreateInput9set_basisERKNSt6stringE)
      * [`set_large_basis()`](classCreateInput.md#_CPPv4N11CreateInput15set_large_basisERKNSt6stringE)
      * [`set_solvent()`](classCreateInput.md#_CPPv4N11CreateInput11set_solventERKNSt6stringERKNSt6stringE)
      * [`set_print_level()`](classCreateInput.md#_CPPv4N11CreateInput15set_print_levelERKNSt6stringE)
      * [`set_extra_keywords()`](classCreateInput.md#_CPPv4N11CreateInput18set_extra_keywordsERKNSt6stringE)
      * [`set_molecular_specs()`](classCreateInput.md#_CPPv4N11CreateInput19set_molecular_specsEii)
      * [`set_tail()`](classCreateInput.md#_CPPv4N11CreateInput8set_tailERKNSt6stringE)
      * [`set_extension()`](classCreateInput.md#_CPPv4N11CreateInput13set_extensionERKNSt6stringE)
      * [`set_tschk_path()`](classCreateInput.md#_CPPv4N11CreateInput14set_tschk_pathERKNSt6stringE)
      * [`set_freeze_atoms()`](classCreateInput.md#_CPPv4N11CreateInput16set_freeze_atomsEii)
      * [`set_scf_maxcycle()`](classCreateInput.md#_CPPv4N11CreateInput16set_scf_maxcycleEi)
      * [`set_opt_maxcycles()`](classCreateInput.md#_CPPv4N11CreateInput17set_opt_maxcyclesEi)
      * [`set_irc_maxpoints()`](classCreateInput.md#_CPPv4N11CreateInput17set_irc_maxpointsEi)
      * [`set_irc_recalc()`](classCreateInput.md#_CPPv4N11CreateInput14set_irc_recalcEi)
      * [`set_irc_maxcycle()`](classCreateInput.md#_CPPv4N11CreateInput16set_irc_maxcycleEi)
      * [`set_irc_stepsize()`](classCreateInput.md#_CPPv4N11CreateInput16set_irc_stepsizeEi)
      * [`print_summary()`](classCreateInput.md#_CPPv4N11CreateInput13print_summaryERK13CreateSummaryRKNSt6stringE)

* [Class FileHandleManager](classFileHandleManager.md)
  * [Nested Relationships](classFileHandleManager.md#nested-relationships)
    * [Nested Types](classFileHandleManager.md#nested-types)
  * [Class Documentation](classFileHandleManager.md#class-documentation)
    * [`FileHandleManager`](classFileHandleManager.md#_CPPv417FileHandleManager)
      * [`acquire()`](classFileHandleManager.md#_CPPv4N17FileHandleManager7acquireEv)
      * [`release()`](classFileHandleManager.md#_CPPv4N17FileHandleManager7releaseEv)
      * [`FileHandleManager::FileGuard`](classFileHandleManager.md#_CPPv4N17FileHandleManager9FileGuardE)
        * [`FileGuard()`](classFileHandleManager.md#_CPPv4N17FileHandleManager9FileGuard9FileGuardEP17FileHandleManager)
        * [`~FileGuard()`](classFileHandleManager.md#_CPPv4N17FileHandleManager9FileGuardD0Ev)
        * [`FileGuard()`](classFileHandleManager.md#_CPPv4N17FileHandleManager9FileGuard9FileGuardERK9FileGuard)
        * [`operator=()`](classFileHandleManager.md#_CPPv4N17FileHandleManager9FileGuardaSERK9FileGuard)
        * [`FileGuard()`](classFileHandleManager.md#_CPPv4N17FileHandleManager9FileGuard9FileGuardERR9FileGuard)
        * [`operator=()`](classFileHandleManager.md#_CPPv4N17FileHandleManager9FileGuardaSERR9FileGuard)
        * [`is_acquired()`](classFileHandleManager.md#_CPPv4NK17FileHandleManager9FileGuard11is_acquiredEv)

* [Class FileHandleManager::FileGuard](classFileHandleManager_1_1FileGuard.md)
  * [Nested Relationships](classFileHandleManager_1_1FileGuard.md#nested-relationships)
  * [Class Documentation](classFileHandleManager_1_1FileGuard.md#class-documentation)
    * [`FileHandleManager::FileGuard`](classFileHandleManager_1_1FileGuard.md#_CPPv4N17FileHandleManager9FileGuardE)
      * [`FileGuard()`](classFileHandleManager_1_1FileGuard.md#_CPPv4N17FileHandleManager9FileGuard9FileGuardEP17FileHandleManager)
      * [`~FileGuard()`](classFileHandleManager_1_1FileGuard.md#_CPPv4N17FileHandleManager9FileGuardD0Ev)
      * [`FileGuard()`](classFileHandleManager_1_1FileGuard.md#_CPPv4N17FileHandleManager9FileGuard9FileGuardERK9FileGuard)
      * [`operator=()`](classFileHandleManager_1_1FileGuard.md#_CPPv4N17FileHandleManager9FileGuardaSERK9FileGuard)
      * [`FileGuard()`](classFileHandleManager_1_1FileGuard.md#_CPPv4N17FileHandleManager9FileGuard9FileGuardERR9FileGuard)
      * [`operator=()`](classFileHandleManager_1_1FileGuard.md#_CPPv4N17FileHandleManager9FileGuardaSERR9FileGuard)
      * [`is_acquired()`](classFileHandleManager_1_1FileGuard.md#_CPPv4NK17FileHandleManager9FileGuard11is_acquiredEv)

* [Class HighLevelEnergyCalculator](classHighLevelEnergyCalculator.md)
  * [Class Documentation](classHighLevelEnergyCalculator.md#class-documentation)
    * [`HighLevelEnergyCalculator`](classHighLevelEnergyCalculator.md#_CPPv425HighLevelEnergyCalculator)
      * [`HighLevelEnergyCalculator()`](classHighLevelEnergyCalculator.md#_CPPv4N25HighLevelEnergyCalculator25HighLevelEnergyCalculatorEddib)
      * [`HighLevelEnergyCalculator()`](classHighLevelEnergyCalculator.md#_CPPv4N25HighLevelEnergyCalculator25HighLevelEnergyCalculatorENSt10shared_ptrI17ProcessingContextEEddib)

* [Class JobChecker](classJobChecker.md)
  * [Class Documentation](classJobChecker.md#class-documentation)
    * [`JobChecker`](classJobChecker.md#_CPPv410JobChecker)
      * [`JobChecker()`](classJobChecker.md#_CPPv4N10JobChecker10JobCheckerENSt10shared_ptrI17ProcessingContextEEbb)

* [Class JobSchedulerDetector](classJobSchedulerDetector.md)
  * [Class Documentation](classJobSchedulerDetector.md#class-documentation)
    * [`JobSchedulerDetector`](classJobSchedulerDetector.md#_CPPv420JobSchedulerDetector)

* [Class MemoryMonitor](classMemoryMonitor.md)
  * [Class Documentation](classMemoryMonitor.md#class-documentation)
    * [`MemoryMonitor`](classMemoryMonitor.md#_CPPv413MemoryMonitor)
      * [`MemoryMonitor()`](classMemoryMonitor.md#_CPPv4N13MemoryMonitor13MemoryMonitorE6size_t)
      * [`can_allocate()`](classMemoryMonitor.md#_CPPv4N13MemoryMonitor12can_allocateE6size_t)
      * [`add_usage()`](classMemoryMonitor.md#_CPPv4N13MemoryMonitor9add_usageE6size_t)
      * [`remove_usage()`](classMemoryMonitor.md#_CPPv4N13MemoryMonitor12remove_usageE6size_t)
      * [`get_current_usage()`](classMemoryMonitor.md#_CPPv4NK13MemoryMonitor17get_current_usageEv)
      * [`get_peak_usage()`](classMemoryMonitor.md#_CPPv4NK13MemoryMonitor14get_peak_usageEv)
      * [`get_max_usage()`](classMemoryMonitor.md#_CPPv4NK13MemoryMonitor13get_max_usageEv)
      * [`set_memory_limit()`](classMemoryMonitor.md#_CPPv4N13MemoryMonitor16set_memory_limitE6size_t)
      * [`get_system_memory_mb()`](classMemoryMonitor.md#_CPPv4N13MemoryMonitor20get_system_memory_mbEv)
      * [`calculate_optimal_memory_limit()`](classMemoryMonitor.md#_CPPv4N13MemoryMonitor30calculate_optimal_memory_limitEj6size_t)

* [Class ParameterParser](classParameterParser.md)
  * [Class Documentation](classParameterParser.md#class-documentation)
    * [`ParameterParser`](classParameterParser.md#_CPPv415ParameterParser)
      * [`ParameterParser()`](classParameterParser.md#_CPPv4N15ParameterParser15ParameterParserEv)
      * [`loadFromFile()`](classParameterParser.md#_CPPv4N15ParameterParser12loadFromFileERKNSt6stringE)
      * [`saveToFile()`](classParameterParser.md#_CPPv4NK15ParameterParser10saveToFileERKNSt6stringE)
      * [`getString()`](classParameterParser.md#_CPPv4NK15ParameterParser9getStringERKNSt6stringERKNSt6stringE)
      * [`getInt()`](classParameterParser.md#_CPPv4NK15ParameterParser6getIntERKNSt6stringEi)
      * [`getBool()`](classParameterParser.md#_CPPv4NK15ParameterParser7getBoolERKNSt6stringEb)
      * [`setString()`](classParameterParser.md#_CPPv4N15ParameterParser9setStringERKNSt6stringERKNSt6stringE)
      * [`setInt()`](classParameterParser.md#_CPPv4N15ParameterParser6setIntERKNSt6stringEi)
      * [`setBool()`](classParameterParser.md#_CPPv4N15ParameterParser7setBoolERKNSt6stringEb)
      * [`hasParameter()`](classParameterParser.md#_CPPv4NK15ParameterParser12hasParameterERKNSt6stringE)
      * [`generateTemplate()`](classParameterParser.md#_CPPv4NK15ParameterParser16generateTemplateERKNSt6stringERKNSt6stringE)
      * [`generateAllTemplates()`](classParameterParser.md#_CPPv4NK15ParameterParser20generateAllTemplatesERKNSt6stringE)
      * [`generateGeneralTemplate()`](classParameterParser.md#_CPPv4NK15ParameterParser23generateGeneralTemplateERKNSt6stringE)
      * [`getSupportedCalcTypes()`](classParameterParser.md#_CPPv4NK15ParameterParser21getSupportedCalcTypesEv)
      * [`clear()`](classParameterParser.md#_CPPv4N15ParameterParser5clearEv)

* [Class ThreadPool](classThreadPool.md)
  * [Class Documentation](classThreadPool.md#class-documentation)
    * [`ThreadPool`](classThreadPool.md#_CPPv410ThreadPool)
      * [`ThreadPool()`](classThreadPool.md#_CPPv4N10ThreadPool10ThreadPoolE6size_t)
      * [`~ThreadPool()`](classThreadPool.md#_CPPv4N10ThreadPoolD0Ev)
      * [`enqueue()`](classThreadPool.md#_CPPv4I0DpEN10ThreadPool7enqueueENSt6futureINSt13invoke_resultI1FDp4ArgsE4typeEEERR1FDpRR4Args)
      * [`active_count()`](classThreadPool.md#_CPPv4NK10ThreadPool12active_countEv)
      * [`wait_for_completion()`](classThreadPool.md#_CPPv4N10ThreadPool19wait_for_completionEv)
      * [`is_shutting_down()`](classThreadPool.md#_CPPv4NK10ThreadPool16is_shutting_downEv)

* [Class ThreadSafeErrorCollector](classThreadSafeErrorCollector.md)
  * [Class Documentation](classThreadSafeErrorCollector.md#class-documentation)
    * [`ThreadSafeErrorCollector`](classThreadSafeErrorCollector.md#_CPPv424ThreadSafeErrorCollector)
      * [`add_error()`](classThreadSafeErrorCollector.md#_CPPv4N24ThreadSafeErrorCollector9add_errorERKNSt6stringE)
      * [`add_warning()`](classThreadSafeErrorCollector.md#_CPPv4N24ThreadSafeErrorCollector11add_warningERKNSt6stringE)
      * [`get_errors()`](classThreadSafeErrorCollector.md#_CPPv4NK24ThreadSafeErrorCollector10get_errorsEv)
      * [`get_warnings()`](classThreadSafeErrorCollector.md#_CPPv4NK24ThreadSafeErrorCollector12get_warningsEv)
      * [`has_errors()`](classThreadSafeErrorCollector.md#_CPPv4NK24ThreadSafeErrorCollector10has_errorsEv)
      * [`clear()`](classThreadSafeErrorCollector.md#_CPPv4N24ThreadSafeErrorCollector5clearEv)

### Enums

* [Enum CalculationType](enum_create__input_8h_1ae2fb950fc8d1f9388248733404704d2f.md)
  * [Enum Documentation](enum_create__input_8h_1ae2fb950fc8d1f9388248733404704d2f.md#enum-documentation)
    * [`CalculationType`](enum_create__input_8h_1ae2fb950fc8d1f9388248733404704d2f.md#_CPPv415CalculationType)
      * [`SP`](enum_create__input_8h_1ae2fb950fc8d1f9388248733404704d2f.md#_CPPv4N15CalculationType2SPE)
      * [`OPT_FREQ`](enum_create__input_8h_1ae2fb950fc8d1f9388248733404704d2f.md#_CPPv4N15CalculationType8OPT_FREQE)
      * [`TS_FREQ`](enum_create__input_8h_1ae2fb950fc8d1f9388248733404704d2f.md#_CPPv4N15CalculationType7TS_FREQE)
      * [`OSS_TS_FREQ`](enum_create__input_8h_1ae2fb950fc8d1f9388248733404704d2f.md#_CPPv4N15CalculationType11OSS_TS_FREQE)
      * [`OSS_CHECK_SP`](enum_create__input_8h_1ae2fb950fc8d1f9388248733404704d2f.md#_CPPv4N15CalculationType12OSS_CHECK_SPE)
      * [`HIGH_SP`](enum_create__input_8h_1ae2fb950fc8d1f9388248733404704d2f.md#_CPPv4N15CalculationType7HIGH_SPE)
      * [`IRC_FORWARD`](enum_create__input_8h_1ae2fb950fc8d1f9388248733404704d2f.md#_CPPv4N15CalculationType11IRC_FORWARDE)
      * [`IRC_REVERSE`](enum_create__input_8h_1ae2fb950fc8d1f9388248733404704d2f.md#_CPPv4N15CalculationType11IRC_REVERSEE)
      * [`IRC`](enum_create__input_8h_1ae2fb950fc8d1f9388248733404704d2f.md#_CPPv4N15CalculationType3IRCE)
      * [`MODRE_TS_FREQ`](enum_create__input_8h_1ae2fb950fc8d1f9388248733404704d2f.md#_CPPv4N15CalculationType13MODRE_TS_FREQE)

* [Enum CommandType](enum_command__system_8h_1a21e038f5b8958e203d28bc4f18472352.md)
  * [Enum Documentation](enum_command__system_8h_1a21e038f5b8958e203d28bc4f18472352.md#enum-documentation)
    * [`CommandType`](enum_command__system_8h_1a21e038f5b8958e203d28bc4f18472352.md#_CPPv411CommandType)
      * [`EXTRACT`](enum_command__system_8h_1a21e038f5b8958e203d28bc4f18472352.md#_CPPv4N11CommandType7EXTRACTE)
      * [`CHECK_DONE`](enum_command__system_8h_1a21e038f5b8958e203d28bc4f18472352.md#_CPPv4N11CommandType10CHECK_DONEE)
      * [`CHECK_ERRORS`](enum_command__system_8h_1a21e038f5b8958e203d28bc4f18472352.md#_CPPv4N11CommandType12CHECK_ERRORSE)
      * [`CHECK_PCM`](enum_command__system_8h_1a21e038f5b8958e203d28bc4f18472352.md#_CPPv4N11CommandType9CHECK_PCME)
      * [`CHECK_IMAGINARY`](enum_command__system_8h_1a21e038f5b8958e203d28bc4f18472352.md#_CPPv4N11CommandType15CHECK_IMAGINARYE)
      * [`CHECK_ALL`](enum_command__system_8h_1a21e038f5b8958e203d28bc4f18472352.md#_CPPv4N11CommandType9CHECK_ALLE)
      * [`HIGH_LEVEL_KJ`](enum_command__system_8h_1a21e038f5b8958e203d28bc4f18472352.md#_CPPv4N11CommandType13HIGH_LEVEL_KJE)
      * [`HIGH_LEVEL_AU`](enum_command__system_8h_1a21e038f5b8958e203d28bc4f18472352.md#_CPPv4N11CommandType13HIGH_LEVEL_AUE)
      * [`EXTRACT_COORDS`](enum_command__system_8h_1a21e038f5b8958e203d28bc4f18472352.md#_CPPv4N11CommandType14EXTRACT_COORDSE)
      * [`CREATE_INPUT`](enum_command__system_8h_1a21e038f5b8958e203d28bc4f18472352.md#_CPPv4N11CommandType12CREATE_INPUTE)

* [Enum ConfigCategory](enum_config__manager_8h_1a32f23b552ade08d96d49cc3061a27165.md)
  * [Enum Documentation](enum_config__manager_8h_1a32f23b552ade08d96d49cc3061a27165.md#enum-documentation)
    * [`ConfigCategory`](enum_config__manager_8h_1a32f23b552ade08d96d49cc3061a27165.md#_CPPv414ConfigCategory)
      * [`GENERAL`](enum_config__manager_8h_1a32f23b552ade08d96d49cc3061a27165.md#_CPPv4N14ConfigCategory7GENERALE)
      * [`EXTRACT`](enum_config__manager_8h_1a32f23b552ade08d96d49cc3061a27165.md#_CPPv4N14ConfigCategory7EXTRACTE)
      * [`JOB_CHECKER`](enum_config__manager_8h_1a32f23b552ade08d96d49cc3061a27165.md#_CPPv4N14ConfigCategory11JOB_CHECKERE)
      * [`PERFORMANCE`](enum_config__manager_8h_1a32f23b552ade08d96d49cc3061a27165.md#_CPPv4N14ConfigCategory11PERFORMANCEE)
      * [`OUTPUT`](enum_config__manager_8h_1a32f23b552ade08d96d49cc3061a27165.md#_CPPv4N14ConfigCategory6OUTPUTE)

* [Enum FileReadMode](enum_utils_8h_1a0dc5721c2ff6b50758dc4f9688ea3cfe.md)
  * [Enum Documentation](enum_utils_8h_1a0dc5721c2ff6b50758dc4f9688ea3cfe.md#enum-documentation)
    * [`FileReadMode`](enum_utils_8h_1a0dc5721c2ff6b50758dc4f9688ea3cfe.md#_CPPv412FileReadMode)
      * [`FULL`](enum_utils_8h_1a0dc5721c2ff6b50758dc4f9688ea3cfe.md#_CPPv4N12FileReadMode4FULLE)
      * [`TAIL`](enum_utils_8h_1a0dc5721c2ff6b50758dc4f9688ea3cfe.md#_CPPv4N12FileReadMode4TAILE)
      * [`SMART`](enum_utils_8h_1a0dc5721c2ff6b50758dc4f9688ea3cfe.md#_CPPv4N12FileReadMode5SMARTE)

* [Enum JobStatus](enum_job__checker_8h_1a5df92147ec0babfdf2aa8856833bb4b8.md)
  * [Enum Documentation](enum_job__checker_8h_1a5df92147ec0babfdf2aa8856833bb4b8.md#enum-documentation)
    * [`JobStatus`](enum_job__checker_8h_1a5df92147ec0babfdf2aa8856833bb4b8.md#_CPPv49JobStatus)
      * [`COMPLETED`](enum_job__checker_8h_1a5df92147ec0babfdf2aa8856833bb4b8.md#_CPPv4N9JobStatus9COMPLETEDE)
      * [`ERROR`](enum_job__checker_8h_1a5df92147ec0babfdf2aa8856833bb4b8.md#_CPPv4N9JobStatus5ERRORE)
      * [`PCM_FAILED`](enum_job__checker_8h_1a5df92147ec0babfdf2aa8856833bb4b8.md#_CPPv4N9JobStatus10PCM_FAILEDE)
      * [`RUNNING`](enum_job__checker_8h_1a5df92147ec0babfdf2aa8856833bb4b8.md#_CPPv4N9JobStatus7RUNNINGE)
      * [`UNKNOWN`](enum_job__checker_8h_1a5df92147ec0babfdf2aa8856833bb4b8.md#_CPPv4N9JobStatus7UNKNOWNE)

* [Enum SchedulerType](enum_job__scheduler_8h_1a342cd0e47545da3e679b393fc548c6b1.md)
  * [Enum Documentation](enum_job__scheduler_8h_1a342cd0e47545da3e679b393fc548c6b1.md#enum-documentation)
    * [`SchedulerType`](enum_job__scheduler_8h_1a342cd0e47545da3e679b393fc548c6b1.md#_CPPv413SchedulerType)
      * [`NONE`](enum_job__scheduler_8h_1a342cd0e47545da3e679b393fc548c6b1.md#_CPPv4N13SchedulerType4NONEE)
      * [`SLURM`](enum_job__scheduler_8h_1a342cd0e47545da3e679b393fc548c6b1.md#_CPPv4N13SchedulerType5SLURME)
      * [`PBS`](enum_job__scheduler_8h_1a342cd0e47545da3e679b393fc548c6b1.md#_CPPv4N13SchedulerType3PBSE)
      * [`SGE`](enum_job__scheduler_8h_1a342cd0e47545da3e679b393fc548c6b1.md#_CPPv4N13SchedulerType3SGEE)
      * [`LSF`](enum_job__scheduler_8h_1a342cd0e47545da3e679b393fc548c6b1.md#_CPPv4N13SchedulerType3LSFE)
      * [`UNKNOWN_CLUSTER`](enum_job__scheduler_8h_1a342cd0e47545da3e679b393fc548c6b1.md#_CPPv4N13SchedulerType15UNKNOWN_CLUSTERE)

### Functions

* [Function calculateSafeMemoryLimit](function_group__ResourceCalculation_1ga92cbfbad3c9f226e74f095b3703edff4.md)
  * [Function Documentation](function_group__ResourceCalculation_1ga92cbfbad3c9f226e74f095b3703edff4.md#function-documentation)

* [Function calculateSafeThreadCount](function_group__ResourceCalculation_1gaf6946ebbf530a167fc82bb7f20b36a42.md)
  * [Function Documentation](function_group__ResourceCalculation_1gaf6946ebbf530a167fc82bb7f20b36a42.md#function-documentation)

* [Function compareResults](function_group__CoreFunctions_1ga185cd895974d0b226a5c66a2f7df8072.md)
  * [Function Documentation](function_group__CoreFunctions_1ga185cd895974d0b226a5c66a2f7df8072.md#function-documentation)

* [Function ConfigUtils::bool_to_string](function_group__ConfigStringUtils_1ga86a587e428778ffc80339b44ab8bc498.md)
  * [Function Documentation](function_group__ConfigStringUtils_1ga86a587e428778ffc80339b44ab8bc498.md#function-documentation)

* [Function ConfigUtils::file_exists](function_group__ConfigFileUtils_1gabdac223165725cc113ecae62b0c10b41.md)
  * [Function Documentation](function_group__ConfigFileUtils_1gabdac223165725cc113ecae62b0c10b41.md#function-documentation)

* [Function ConfigUtils::get_config_search_paths](function_group__ConfigFileUtils_1ga09f057aaa03de2ed7c66d9a0ca26e047.md)
  * [Function Documentation](function_group__ConfigFileUtils_1ga09f057aaa03de2ed7c66d9a0ca26e047.md#function-documentation)

* [Function ConfigUtils::get_config_version](function_group__ConfigMigration_1ga70342c1315c8d4baaa0a5d567f7ebd82.md)
  * [Function Documentation](function_group__ConfigMigration_1ga70342c1315c8d4baaa0a5d567f7ebd82.md#function-documentation)

* [Function ConfigUtils::get_executable_directory](function_group__ConfigFileUtils_1gaf98b0febad38de1b8c8b06b5b9808d26.md)
  * [Function Documentation](function_group__ConfigFileUtils_1gaf98b0febad38de1b8c8b06b5b9808d26.md#function-documentation)

* [Function ConfigUtils::is_readable](function_group__ConfigFileUtils_1ga892ca7155558aa7c9813c0c04ab55a8e.md)
  * [Function Documentation](function_group__ConfigFileUtils_1ga892ca7155558aa7c9813c0c04ab55a8e.md#function-documentation)

* [Function ConfigUtils::is_valid_concentration](function_group__ConfigValidationUtils_1gae4991b82543e777bc3a4b13fba2e2ac2.md)
  * [Function Documentation](function_group__ConfigValidationUtils_1gae4991b82543e777bc3a4b13fba2e2ac2.md#function-documentation)

* [Function ConfigUtils::is_valid_extension](function_group__ConfigValidationUtils_1ga6905a372521e35204369ffa3bb8d8b8e.md)
  * [Function Documentation](function_group__ConfigValidationUtils_1ga6905a372521e35204369ffa3bb8d8b8e.md#function-documentation)

* [Function ConfigUtils::is_valid_file_size](function_group__ConfigValidationUtils_1gabbc0a80ccd4e422868d76a759fad7a48.md)
  * [Function Documentation](function_group__ConfigValidationUtils_1gabbc0a80ccd4e422868d76a759fad7a48.md#function-documentation)

* [Function ConfigUtils::is_valid_pressure](function_group__ConfigValidationUtils_1gaaab1339b93b3b34c3f6c53b8e6a3727e.md)
  * [Function Documentation](function_group__ConfigValidationUtils_1gaaab1339b93b3b34c3f6c53b8e6a3727e.md#function-documentation)

* [Function ConfigUtils::is_valid_temperature](function_group__ConfigValidationUtils_1gab7c7175516bb9e9ba06fbae9929ffdc9.md)
  * [Function Documentation](function_group__ConfigValidationUtils_1gab7c7175516bb9e9ba06fbae9929ffdc9.md#function-documentation)

* [Function ConfigUtils::is_valid_thread_count](function_group__ConfigValidationUtils_1gad9b0eb66fc92a6e8c26ebb2c10a9d051.md)
  * [Function Documentation](function_group__ConfigValidationUtils_1gad9b0eb66fc92a6e8c26ebb2c10a9d051.md#function-documentation)

* [Function ConfigUtils::is_writable](function_group__ConfigFileUtils_1gaf39bd0ce945253316d2d2932e0e52173.md)
  * [Function Documentation](function_group__ConfigFileUtils_1gaf39bd0ce945253316d2d2932e0e52173.md#function-documentation)

* [Function ConfigUtils::join_strings](function_group__ConfigStringUtils_1gacf5b66a58b4f7dd611d76602c8cc9002.md)
  * [Function Documentation](function_group__ConfigStringUtils_1gacf5b66a58b4f7dd611d76602c8cc9002.md#function-documentation)

* [Function ConfigUtils::migrate_config_if_needed](function_group__ConfigMigration_1gafe82a56e2d192ad97f3cad5815add706.md)
  * [Function Documentation](function_group__ConfigMigration_1gafe82a56e2d192ad97f3cad5815add706.md#function-documentation)

* [Function ConfigUtils::split_string](function_group__ConfigStringUtils_1ga2f21572e2bdf739cbd8c75720728196c.md)
  * [Function Documentation](function_group__ConfigStringUtils_1ga2f21572e2bdf739cbd8c75720728196c.md#function-documentation)

* [Function ConfigUtils::string_to_bool](function_group__ConfigStringUtils_1ga49ee140641a23b58f60a06e016fca429.md)
  * [Function Documentation](function_group__ConfigStringUtils_1ga49ee140641a23b58f60a06e016fca429.md#function-documentation)

* [Function execute_check_all_command(const CommandContext&)](function_interactive__mode_8h_1a5f1a9bdbc166546e200c5bbb2aa5db30.md)
  * [Function Documentation](function_interactive__mode_8h_1a5f1a9bdbc166546e200c5bbb2aa5db30.md#function-documentation)
    * [`execute_check_all_command()`](function_interactive__mode_8h_1a5f1a9bdbc166546e200c5bbb2aa5db30.md#_CPPv425execute_check_all_commandRK14CommandContext)

* [Function execute_check_all_command()](function_group__ModuleExecutors_1ga5f1a9bdbc166546e200c5bbb2aa5db30.md)
  * [Function Documentation](function_group__ModuleExecutors_1ga5f1a9bdbc166546e200c5bbb2aa5db30.md#function-documentation)

* [Function execute_check_done_command(const CommandContext&)](function_interactive__mode_8h_1a97d4d0bad6dab32f1d5b92d4ea09c80b.md)
  * [Function Documentation](function_interactive__mode_8h_1a97d4d0bad6dab32f1d5b92d4ea09c80b.md#function-documentation)
    * [`execute_check_done_command()`](function_interactive__mode_8h_1a97d4d0bad6dab32f1d5b92d4ea09c80b.md#_CPPv426execute_check_done_commandRK14CommandContext)

* [Function execute_check_done_command()](function_group__ModuleExecutors_1ga97d4d0bad6dab32f1d5b92d4ea09c80b.md)
  * [Function Documentation](function_group__ModuleExecutors_1ga97d4d0bad6dab32f1d5b92d4ea09c80b.md#function-documentation)

* [Function execute_check_errors_command(const CommandContext&)](function_interactive__mode_8h_1a530afea0ae66ae1aef943507742b3125.md)
  * [Function Documentation](function_interactive__mode_8h_1a530afea0ae66ae1aef943507742b3125.md#function-documentation)
    * [`execute_check_errors_command()`](function_interactive__mode_8h_1a530afea0ae66ae1aef943507742b3125.md#_CPPv428execute_check_errors_commandRK14CommandContext)

* [Function execute_check_errors_command()](function_group__ModuleExecutors_1ga530afea0ae66ae1aef943507742b3125.md)
  * [Function Documentation](function_group__ModuleExecutors_1ga530afea0ae66ae1aef943507742b3125.md#function-documentation)

* [Function execute_check_imaginary_command(const CommandContext&)](function_interactive__mode_8h_1a4f5c061099b04712ebc615b32108fdd5.md)
  * [Function Documentation](function_interactive__mode_8h_1a4f5c061099b04712ebc615b32108fdd5.md#function-documentation)
    * [`execute_check_imaginary_command()`](function_interactive__mode_8h_1a4f5c061099b04712ebc615b32108fdd5.md#_CPPv431execute_check_imaginary_commandRK14CommandContext)

* [Function execute_check_imaginary_command()](function_group__ModuleExecutors_1ga4f5c061099b04712ebc615b32108fdd5.md)
  * [Function Documentation](function_group__ModuleExecutors_1ga4f5c061099b04712ebc615b32108fdd5.md#function-documentation)

* [Function execute_check_pcm_command(const CommandContext&)](function_interactive__mode_8h_1a02765a7766f40fab78024f30a1b3f9ff.md)
  * [Function Documentation](function_interactive__mode_8h_1a02765a7766f40fab78024f30a1b3f9ff.md#function-documentation)
    * [`execute_check_pcm_command()`](function_interactive__mode_8h_1a02765a7766f40fab78024f30a1b3f9ff.md#_CPPv425execute_check_pcm_commandRK14CommandContext)

* [Function execute_check_pcm_command()](function_group__ModuleExecutors_1ga02765a7766f40fab78024f30a1b3f9ff.md)
  * [Function Documentation](function_group__ModuleExecutors_1ga02765a7766f40fab78024f30a1b3f9ff.md#function-documentation)

* [Function execute_create_input_command(const CommandContext&)](function_interactive__mode_8h_1a6d8429398cd99549f485628a27b0623a.md)
  * [Function Documentation](function_interactive__mode_8h_1a6d8429398cd99549f485628a27b0623a.md#function-documentation)
    * [`execute_create_input_command()`](function_interactive__mode_8h_1a6d8429398cd99549f485628a27b0623a.md#_CPPv428execute_create_input_commandRK14CommandContext)

* [Function execute_create_input_command()](function_group__ModuleExecutors_1ga6d8429398cd99549f485628a27b0623a.md)
  * [Function Documentation](function_group__ModuleExecutors_1ga6d8429398cd99549f485628a27b0623a.md#function-documentation)

* [Function execute_directory_command](function_interactive__mode_8h_1a14f3f6b42735308c6063a97c3bb11279.md)
  * [Function Documentation](function_interactive__mode_8h_1a14f3f6b42735308c6063a97c3bb11279.md#function-documentation)
    * [`execute_directory_command()`](function_interactive__mode_8h_1a14f3f6b42735308c6063a97c3bb11279.md#_CPPv425execute_directory_commandRKNSt6stringE)

* [Function execute_extract_command(const CommandContext&)](function_interactive__mode_8h_1aa356f7897fc875da426df1cf3c9c5b99.md)
  * [Function Documentation](function_interactive__mode_8h_1aa356f7897fc875da426df1cf3c9c5b99.md#function-documentation)
    * [`execute_extract_command()`](function_interactive__mode_8h_1aa356f7897fc875da426df1cf3c9c5b99.md#_CPPv423execute_extract_commandRK14CommandContext)

* [Function execute_extract_command()](function_group__ModuleExecutors_1gaa356f7897fc875da426df1cf3c9c5b99.md)
  * [Function Documentation](function_group__ModuleExecutors_1gaa356f7897fc875da426df1cf3c9c5b99.md#function-documentation)

* [Function execute_extract_coords_command(const CommandContext&)](function_interactive__mode_8h_1ab27343ef2ae55f92b21f39e17f788f4c.md)
  * [Function Documentation](function_interactive__mode_8h_1ab27343ef2ae55f92b21f39e17f788f4c.md#function-documentation)
    * [`execute_extract_coords_command()`](function_interactive__mode_8h_1ab27343ef2ae55f92b21f39e17f788f4c.md#_CPPv430execute_extract_coords_commandRK14CommandContext)

* [Function execute_extract_coords_command()](function_group__ModuleExecutors_1gab27343ef2ae55f92b21f39e17f788f4c.md)
  * [Function Documentation](function_group__ModuleExecutors_1gab27343ef2ae55f92b21f39e17f788f4c.md#function-documentation)

* [Function execute_high_level_au_command(const CommandContext&)](function_interactive__mode_8h_1ab1aa2594a8bb3d2789d37593f69cbc5e.md)
  * [Function Documentation](function_interactive__mode_8h_1ab1aa2594a8bb3d2789d37593f69cbc5e.md#function-documentation)
    * [`execute_high_level_au_command()`](function_interactive__mode_8h_1ab1aa2594a8bb3d2789d37593f69cbc5e.md#_CPPv429execute_high_level_au_commandRK14CommandContext)

* [Function execute_high_level_au_command()](function_group__ModuleExecutors_1gab1aa2594a8bb3d2789d37593f69cbc5e.md)
  * [Function Documentation](function_group__ModuleExecutors_1gab1aa2594a8bb3d2789d37593f69cbc5e.md#function-documentation)

* [Function execute_high_level_kj_command(const CommandContext&)](function_interactive__mode_8h_1aefd8d559f7f798aaf8cb5f6feb9dce47.md)
  * [Function Documentation](function_interactive__mode_8h_1aefd8d559f7f798aaf8cb5f6feb9dce47.md#function-documentation)
    * [`execute_high_level_kj_command()`](function_interactive__mode_8h_1aefd8d559f7f798aaf8cb5f6feb9dce47.md#_CPPv429execute_high_level_kj_commandRK14CommandContext)

* [Function execute_high_level_kj_command()](function_group__ModuleExecutors_1gaefd8d559f7f798aaf8cb5f6feb9dce47.md)
  * [Function Documentation](function_group__ModuleExecutors_1gaefd8d559f7f798aaf8cb5f6feb9dce47.md#function-documentation)

* [Function execute_utility_command](function_interactive__mode_8h_1a92486af52b1c217ef1affb1e8d9a6d72.md)
  * [Function Documentation](function_interactive__mode_8h_1a92486af52b1c217ef1affb1e8d9a6d72.md#function-documentation)
    * [`execute_utility_command()`](function_interactive__mode_8h_1a92486af52b1c217ef1affb1e8d9a6d72.md#_CPPv423execute_utility_commandRKNSt6stringE)

* [Function extract](function_group__CoreFunctions_1gad17c8399509c1c8b6d29fde9d0bcfac3.md)
  * [Function Documentation](function_group__CoreFunctions_1gad17c8399509c1c8b6d29fde9d0bcfac3.md#function-documentation)

* [Function findLogFiles()](function_group__UtilityFunctions_1gaf62947f0321d444f7e4e26cbc7d1bf8f.md)
  * [Function Documentation](function_group__UtilityFunctions_1gaf62947f0321d444f7e4e26cbc7d1bf8f.md#function-documentation)

* [Function findLogFiles()](function_group__UtilityFunctions_1ga8dcf9bf3a43b711cb268f2ee8948b47c.md)
  * [Function Documentation](function_group__UtilityFunctions_1ga8dcf9bf3a43b711cb268f2ee8948b47c.md#function-documentation)

* [Function findLogFiles()](function_group__UtilityFunctions_1ga9a325bd1793b5dc5e9490383802ad169.md)
  * [Function Documentation](function_group__UtilityFunctions_1ga9a325bd1793b5dc5e9490383802ad169.md#function-documentation)

* [Function findLogFiles()](function_group__UtilityFunctions_1ga5d1bf1ec351f0a9912a810ebe82a1e0a.md)
  * [Function Documentation](function_group__UtilityFunctions_1ga5d1bf1ec351f0a9912a810ebe82a1e0a.md#function-documentation)

* [Function formatMemorySize](function_group__UtilityFunctions_1gad9472b23817cf18d4ae5c857afb94c5f.md)
  * [Function Documentation](function_group__UtilityFunctions_1gad9472b23817cf18d4ae5c857afb94c5f.md#function-documentation)

* [Function GaussianExtractor::get_full_version](function_group__VersionFunctions_1gac6e31792810c344c5d0bc503374bf8d6.md)
  * [Function Documentation](function_group__VersionFunctions_1gac6e31792810c344c5d0bc503374bf8d6.md#function-documentation)

* [Function GaussianExtractor::get_header_info](function_group__VersionFunctions_1ga546fbcfeb96704c283dd3b0d080daf11.md)
  * [Function Documentation](function_group__VersionFunctions_1ga546fbcfeb96704c283dd3b0d080daf11.md#function-documentation)

* [Function GaussianExtractor::get_version](function_group__VersionFunctions_1gaa7d0f6f380da3b7f8fbd8433cf85a52b.md)
  * [Function Documentation](function_group__VersionFunctions_1gaa7d0f6f380da3b7f8fbd8433cf85a52b.md#function-documentation)

* [Function GaussianExtractor::get_version_info](function_group__VersionFunctions_1gaec16789b9c0ace13484f2a7d587dcf55.md)
  * [Function Documentation](function_group__VersionFunctions_1gaec16789b9c0ace13484f2a7d587dcf55.md#function-documentation)

* [Function GaussianExtractor::is_version_at_least](function_group__VersionFunctions_1ga79afbfd2981f3ac46f43516895d8d5fe.md)
  * [Function Documentation](function_group__VersionFunctions_1ga79afbfd2981f3ac46f43516895d8d5fe.md#function-documentation)

* [Function HelpUtils::create_default_config](function_namespaceHelpUtils_1aa496c9db09fcfa776df8300da1cc2711.md)
  * [Function Documentation](function_namespaceHelpUtils_1aa496c9db09fcfa776df8300da1cc2711.md#function-documentation)
    * [`create_default_config()`](function_namespaceHelpUtils_1aa496c9db09fcfa776df8300da1cc2711.md#_CPPv4N9HelpUtils21create_default_configEv)

* [Function HelpUtils::print_command_help](function_namespaceHelpUtils_1a414c98dd658dfaeb81f4a04b8cd69ef5.md)
  * [Function Documentation](function_namespaceHelpUtils_1a414c98dd658dfaeb81f4a04b8cd69ef5.md#function-documentation)
    * [`print_command_help()`](function_namespaceHelpUtils_1a414c98dd658dfaeb81f4a04b8cd69ef5.md#_CPPv4N9HelpUtils18print_command_helpE11CommandTypeRKNSt6stringE)

* [Function HelpUtils::print_config_help](function_namespaceHelpUtils_1a2fb76d8b3311bed23a95981b9f07c135.md)
  * [Function Documentation](function_namespaceHelpUtils_1a2fb76d8b3311bed23a95981b9f07c135.md#function-documentation)
    * [`print_config_help()`](function_namespaceHelpUtils_1a2fb76d8b3311bed23a95981b9f07c135.md#_CPPv4N9HelpUtils17print_config_helpEv)

* [Function HelpUtils::print_help](function_namespaceHelpUtils_1a1fceb380d0b85630363042aab2a46b25.md)
  * [Function Documentation](function_namespaceHelpUtils_1a1fceb380d0b85630363042aab2a46b25.md#function-documentation)
    * [`print_help()`](function_namespaceHelpUtils_1a1fceb380d0b85630363042aab2a46b25.md#_CPPv4N9HelpUtils10print_helpERKNSt6stringE)

* [Function HighLevelEnergyUtils::extract_frequencies](function_group__EnergyParsingUtils_1ga95f6c769f275d3b7659d6f379b106b51.md)
  * [Function Documentation](function_group__EnergyParsingUtils_1ga95f6c769f275d3b7659d6f379b106b51.md#function-documentation)

* [Function HighLevelEnergyUtils::find_lowest_frequency](function_group__EnergyParsingUtils_1gaf1573e500ba7a88a65e5ccf978c00621.md)
  * [Function Documentation](function_group__EnergyParsingUtils_1gaf1573e500ba7a88a65e5ccf978c00621.md#function-documentation)

* [Function HighLevelEnergyUtils::get_current_directory_name](function_group__FileDirectoryUtils_1gaf8e0b2ee0b197f1fd5036b75f7d1a9fd.md)
  * [Function Documentation](function_group__FileDirectoryUtils_1gaf8e0b2ee0b197f1fd5036b75f7d1a9fd.md#function-documentation)

* [Function HighLevelEnergyUtils::hartree_to_ev](function_group__ConversionUtils_1gab18e95c3b82fcdbcecec7b278493157c.md)
  * [Function Documentation](function_group__ConversionUtils_1gab18e95c3b82fcdbcecec7b278493157c.md#function-documentation)

* [Function HighLevelEnergyUtils::hartree_to_kj_mol](function_group__ConversionUtils_1ga25dfe5039543fe129bedb1e0892be135.md)
  * [Function Documentation](function_group__ConversionUtils_1ga25dfe5039543fe129bedb1e0892be135.md#function-documentation)

* [Function HighLevelEnergyUtils::is_valid_high_level_directory()](function_group__FileDirectoryUtils_1ga9851a9bb4e49d1d90f71249aeebc61e6.md)
  * [Function Documentation](function_group__FileDirectoryUtils_1ga9851a9bb4e49d1d90f71249aeebc61e6.md#function-documentation)

* [Function HighLevelEnergyUtils::is_valid_high_level_directory()](function_group__FileDirectoryUtils_1gac1ff563cdfa290008403d17b0d2a91cd.md)
  * [Function Documentation](function_group__FileDirectoryUtils_1gac1ff563cdfa290008403d17b0d2a91cd.md#function-documentation)

* [Function HighLevelEnergyUtils::parse_energy_value](function_group__EnergyParsingUtils_1ga0fa63ff04b8e0794254fe2546859f39d.md)
  * [Function Documentation](function_group__EnergyParsingUtils_1ga0fa63ff04b8e0794254fe2546859f39d.md#function-documentation)

* [Function HighLevelEnergyUtils::validate_concentration](function_group__ValidationUtils_1ga06764cec412d21151dc74bb9106095a1.md)
  * [Function Documentation](function_group__ValidationUtils_1ga06764cec412d21151dc74bb9106095a1.md#function-documentation)

* [Function HighLevelEnergyUtils::validate_energy_data](function_group__ValidationUtils_1gabe3d651ced16adc942b77bd92be3116a.md)
  * [Function Documentation](function_group__ValidationUtils_1gabe3d651ced16adc942b77bd92be3116a.md#function-documentation)

* [Function HighLevelEnergyUtils::validate_temperature](function_group__ValidationUtils_1gad41dd357e75a3c0219b5ed3a6b964aa6.md)
  * [Function Documentation](function_group__ValidationUtils_1gad41dd357e75a3c0219b5ed3a6b964aa6.md#function-documentation)

* [Function is_directory_command](function_interactive__mode_8h_1a7df3c083f83183ac64c8bfa5af8e0846.md)
  * [Function Documentation](function_interactive__mode_8h_1a7df3c083f83183ac64c8bfa5af8e0846.md#function-documentation)
    * [`is_directory_command()`](function_interactive__mode_8h_1a7df3c083f83183ac64c8bfa5af8e0846.md#_CPPv420is_directory_commandRKNSt6stringE)

* [Function is_shell_command](function_interactive__mode_8h_1a56f663d093a1a92e51d05f4f93d694fe.md)
  * [Function Documentation](function_interactive__mode_8h_1a56f663d093a1a92e51d05f4f93d694fe.md#function-documentation)
    * [`is_shell_command()`](function_interactive__mode_8h_1a56f663d093a1a92e51d05f4f93d694fe.md#_CPPv416is_shell_commandRKNSt6stringE)

* [Function is_utility_command](function_interactive__mode_8h_1af98c7951f5700d61e330561219d419fa.md)
  * [Function Documentation](function_interactive__mode_8h_1af98c7951f5700d61e330561219d419fa.md#function-documentation)
    * [`is_utility_command()`](function_interactive__mode_8h_1af98c7951f5700d61e330561219d419fa.md#_CPPv418is_utility_commandRKNSt6stringE)

* [Function JobCheckerUtils::file_exists](function_group__JobFileUtils_1ga93b2603d1ed51dcb6ad00bb6c48a434b.md)
  * [Function Documentation](function_group__JobFileUtils_1ga93b2603d1ed51dcb6ad00bb6c48a434b.md#function-documentation)

* [Function JobCheckerUtils::get_file_extension](function_group__JobFileUtils_1ga0022919f724153d514357c39b4658063.md)
  * [Function Documentation](function_group__JobFileUtils_1ga0022919f724153d514357c39b4658063.md#function-documentation)

* [Function JobCheckerUtils::is_valid_log_file](function_group__JobFileUtils_1ga2946f175c06fc2a63a822b44db814ffa.md)
  * [Function Documentation](function_group__JobFileUtils_1ga2946f175c06fc2a63a822b44db814ffa.md#function-documentation)

* [Function Metadata::header](function_namespaceMetadata_1a4447a0ff55fb6119acf046c55f0729d8.md)
  * [Function Documentation](function_namespaceMetadata_1a4447a0ff55fb6119acf046c55f0729d8.md#function-documentation)
    * [`header()`](function_namespaceMetadata_1a4447a0ff55fb6119acf046c55f0729d8.md#_CPPv4N8Metadata6headerEv)

* [Function printJobResourceInfo](function_group__UtilityFunctions_1gad797e43f6528ebd1a5288f01262ccc3d.md)
  * [Function Documentation](function_group__UtilityFunctions_1gad797e43f6528ebd1a5288f01262ccc3d.md#function-documentation)

* [Function printResourceUsage](function_group__UtilityFunctions_1ga13a0fe5daedc9937f2e60e35b862674b.md)
  * [Function Documentation](function_group__UtilityFunctions_1ga13a0fe5daedc9937f2e60e35b862674b.md#function-documentation)

* [Function processAndOutputResults](function_group__CoreFunctions_1ga4a40f96b211c992d0cd35f99d390d13f.md)
  * [Function Documentation](function_group__CoreFunctions_1ga4a40f96b211c992d0cd35f99d390d13f.md#function-documentation)

* [Function run_interactive_loop](function_interactive__mode_8h_1a6b615c34054150d837dc45ed1c17b297.md)
  * [Function Documentation](function_interactive__mode_8h_1a6b615c34054150d837dc45ed1c17b297.md#function-documentation)
    * [`run_interactive_loop()`](function_interactive__mode_8h_1a6b615c34054150d837dc45ed1c17b297.md#_CPPv420run_interactive_loopv)

* [Function safe_stod](function_group__SafeParsing_1ga131e6ac5862047eda88e31ac535d9229.md)
  * [Function Documentation](function_group__SafeParsing_1ga131e6ac5862047eda88e31ac535d9229.md#function-documentation)

* [Function safe_stoi](function_group__SafeParsing_1ga9d2e1dc30c07adc96e7fb857f208bef6.md)
  * [Function Documentation](function_group__SafeParsing_1ga9d2e1dc30c07adc96e7fb857f208bef6.md#function-documentation)

* [Function safe_stoul](function_group__SafeParsing_1ga55358cf328c6698059a9ce209d7a7727.md)
  * [Function Documentation](function_group__SafeParsing_1ga55358cf328c6698059a9ce209d7a7727.md#function-documentation)

* [Function Utils::generate_unique_filename](function_namespaceUtils_1acba799749eeee8229550094df96d9f0d.md)
  * [Function Documentation](function_namespaceUtils_1acba799749eeee8229550094df96d9f0d.md#function-documentation)
    * [`generate_unique_filename()`](function_namespaceUtils_1acba799749eeee8229550094df96d9f0d.md#_CPPv4N5Utils24generate_unique_filenameERKNSt10filesystem4pathE)

* [Function Utils::read_file_unified](function_namespaceUtils_1a438037dac8d7b9a6f17c9cdbe4bfd27f.md)
  * [Function Documentation](function_namespaceUtils_1a438037dac8d7b9a6f17c9cdbe4bfd27f.md#function-documentation)
    * [`read_file_unified()`](function_namespaceUtils_1a438037dac8d7b9a6f17c9cdbe4bfd27f.md#_CPPv4N5Utils17read_file_unifiedERKNSt6stringE12FileReadMode6size_tRKNSt6stringE)

* [Function validateFileSize](function_group__UtilityFunctions_1ga98551a8875f12c1ace388443122c58d9.md)
  * [Function Documentation](function_group__UtilityFunctions_1ga98551a8875f12c1ace388443122c58d9.md#function-documentation)

### Variables

* [Variable ALT_CONFIG_FILENAME](variable_group__ConfigConstants_1ga474675948b88b570377dde986b831027.md)
  * [Variable Documentation](variable_group__ConfigConstants_1ga474675948b88b570377dde986b831027.md#variable-documentation)
    * [`ALT_CONFIG_FILENAME`](variable_group__ConfigConstants_1ga474675948b88b570377dde986b831027.md#_CPPv419ALT_CONFIG_FILENAME)

* [Variable DEFAULT_CONFIG_FILENAME](variable_group__ConfigConstants_1gaf1c392dbe5c126556ae7fdcfcf13f312.md)
  * [Variable Documentation](variable_group__ConfigConstants_1gaf1c392dbe5c126556ae7fdcfcf13f312.md#variable-documentation)
    * [`DEFAULT_CONFIG_FILENAME`](variable_group__ConfigConstants_1gaf1c392dbe5c126556ae7fdcfcf13f312.md#_CPPv423DEFAULT_CONFIG_FILENAME)

* [Variable DEFAULT_MAX_FILE_SIZE_MB](variable_group__SafetyLimits_1ga53a9c91d48aea3f132ce5aac13b8ae8a.md)
  * [Variable Documentation](variable_group__SafetyLimits_1ga53a9c91d48aea3f132ce5aac13b8ae8a.md#variable-documentation)
    * [`DEFAULT_MAX_FILE_SIZE_MB`](variable_group__SafetyLimits_1ga53a9c91d48aea3f132ce5aac13b8ae8a.md#_CPPv424DEFAULT_MAX_FILE_SIZE_MB)

* [Variable DEFAULT_MEMORY_MB](variable_group__SafetyLimits_1ga5b473519c702109b67a50e1c68aa62da.md)
  * [Variable Documentation](variable_group__SafetyLimits_1ga5b473519c702109b67a50e1c68aa62da.md#variable-documentation)
    * [`DEFAULT_MEMORY_MB`](variable_group__SafetyLimits_1ga5b473519c702109b67a50e1c68aa62da.md#_CPPv417DEFAULT_MEMORY_MB)

* [Variable g_config_manager](variable_config__manager_8h_1a5dc02d7cc2dae80387b656e7d050e86c.md)
  * [Variable Documentation](variable_config__manager_8h_1a5dc02d7cc2dae80387b656e7d050e86c.md#variable-documentation)
    * [`g_config_manager`](variable_config__manager_8h_1a5dc02d7cc2dae80387b656e7d050e86c.md#_CPPv416g_config_manager)

* [Variable g_shutdown_requested](variable_gaussian__extractor_8h_1ad9223bbbed1d71323a789daf88911c4a.md)
  * [Variable Documentation](variable_gaussian__extractor_8h_1ad9223bbbed1d71323a789daf88911c4a.md#variable-documentation)
    * [`g_shutdown_requested`](variable_gaussian__extractor_8h_1ad9223bbbed1d71323a789daf88911c4a.md#_CPPv420g_shutdown_requested)

* [Variable HARTREE_TO_EV](variable_group__HighLevelConstants_1gacbe99bd7614c1c4802e6558cc26f19b2.md)
  * [Variable Documentation](variable_group__HighLevelConstants_1gacbe99bd7614c1c4802e6558cc26f19b2.md#variable-documentation)
    * [`HARTREE_TO_EV`](variable_group__HighLevelConstants_1gacbe99bd7614c1c4802e6558cc26f19b2.md#_CPPv413HARTREE_TO_EV)

* [Variable HARTREE_TO_KJ_MOL](variable_group__HighLevelConstants_1ga8887635cc694ff13866b02eaa09d79f8.md)
  * [Variable Documentation](variable_group__HighLevelConstants_1ga8887635cc694ff13866b02eaa09d79f8.md#variable-documentation)
    * [`HARTREE_TO_KJ_MOL`](variable_group__HighLevelConstants_1ga8887635cc694ff13866b02eaa09d79f8.md#_CPPv417HARTREE_TO_KJ_MOL)

* [Variable kB](variable_group__PhysicsConstants_1ga002422f5b9b60e223b4f38505877f8a3.md)
  * [Variable Documentation](variable_group__PhysicsConstants_1ga002422f5b9b60e223b4f38505877f8a3.md#variable-documentation)
    * [`kB`](variable_group__PhysicsConstants_1ga002422f5b9b60e223b4f38505877f8a3.md#_CPPv42kB)

* [Variable KB_CONSTANT](variable_group__HighLevelConstants_1ga236551f48f2e8cc7b63a54e77a307b37.md)
  * [Variable Documentation](variable_group__HighLevelConstants_1ga236551f48f2e8cc7b63a54e77a307b37.md#variable-documentation)
    * [`KB_CONSTANT`](variable_group__HighLevelConstants_1ga236551f48f2e8cc7b63a54e77a307b37.md#_CPPv411KB_CONSTANT)

* [Variable MAX_FILE_HANDLES](variable_group__SafetyLimits_1ga85a5c7ea2679e0f92b11d8de33d02c75.md)
  * [Variable Documentation](variable_group__SafetyLimits_1ga85a5c7ea2679e0f92b11d8de33d02c75.md#variable-documentation)
    * [`MAX_FILE_HANDLES`](variable_group__SafetyLimits_1ga85a5c7ea2679e0f92b11d8de33d02c75.md#_CPPv416MAX_FILE_HANDLES)

* [Variable MAX_MEMORY_MB](variable_group__SafetyLimits_1ga4ae9d2d4ae994dd75f2dbd0b0ce593f3.md)
  * [Variable Documentation](variable_group__SafetyLimits_1ga4ae9d2d4ae994dd75f2dbd0b0ce593f3.md#variable-documentation)
    * [`MAX_MEMORY_MB`](variable_group__SafetyLimits_1ga4ae9d2d4ae994dd75f2dbd0b0ce593f3.md#_CPPv413MAX_MEMORY_MB)

* [Variable MIN_MEMORY_MB](variable_group__SafetyLimits_1ga99f41eb90c4eb0abba0d99171b29aa40.md)
  * [Variable Documentation](variable_group__SafetyLimits_1ga99f41eb90c4eb0abba0d99171b29aa40.md#variable-documentation)
    * [`MIN_MEMORY_MB`](variable_group__SafetyLimits_1ga99f41eb90c4eb0abba0d99171b29aa40.md#_CPPv413MIN_MEMORY_MB)

* [Variable PHASE_CORR_FACTOR](variable_group__HighLevelConstants_1gad01e819322d383bd0c10b0f9ab5652bb.md)
  * [Variable Documentation](variable_group__HighLevelConstants_1gad01e819322d383bd0c10b0f9ab5652bb.md#variable-documentation)
    * [`PHASE_CORR_FACTOR`](variable_group__HighLevelConstants_1gad01e819322d383bd0c10b0f9ab5652bb.md#_CPPv417PHASE_CORR_FACTOR)

* [Variable Po](variable_group__PhysicsConstants_1ga2af4da1ed0b3553264e3d0b583f56277.md)
  * [Variable Documentation](variable_group__PhysicsConstants_1ga2af4da1ed0b3553264e3d0b583f56277.md#variable-documentation)
    * [`Po`](variable_group__PhysicsConstants_1ga2af4da1ed0b3553264e3d0b583f56277.md#_CPPv42Po)

* [Variable PO_CONSTANT](variable_group__HighLevelConstants_1ga8d037406545289fa70a65054a64a6f0b.md)
  * [Variable Documentation](variable_group__HighLevelConstants_1ga8d037406545289fa70a65054a64a6f0b.md#variable-documentation)
    * [`PO_CONSTANT`](variable_group__HighLevelConstants_1ga8d037406545289fa70a65054a64a6f0b.md#_CPPv411PO_CONSTANT)

* [Variable R](variable_group__PhysicsConstants_1ga0877420f3d7b1f47b871d2ccb47168d8.md)
  * [Variable Documentation](variable_group__PhysicsConstants_1ga0877420f3d7b1f47b871d2ccb47168d8.md#variable-documentation)
    * [`R`](variable_group__PhysicsConstants_1ga0877420f3d7b1f47b871d2ccb47168d8.md#_CPPv41R)

* [Variable R_CONSTANT](variable_group__HighLevelConstants_1ga916c57027cc478cc10d999732b0dca62.md)
  * [Variable Documentation](variable_group__HighLevelConstants_1ga916c57027cc478cc10d999732b0dca62.md#variable-documentation)
    * [`R_CONSTANT`](variable_group__HighLevelConstants_1ga916c57027cc478cc10d999732b0dca62.md#_CPPv410R_CONSTANT)

### Defines

* [Define CONFIG_GET_BOOL](define_group__ConfigMacros_1ga680f26fa7361a2519724a27e6ef9f88f.md)
  * [Define Documentation](define_group__ConfigMacros_1ga680f26fa7361a2519724a27e6ef9f88f.md#define-documentation)
    * [`CONFIG_GET_BOOL`](define_group__ConfigMacros_1ga680f26fa7361a2519724a27e6ef9f88f.md#c.CONFIG_GET_BOOL)

* [Define CONFIG_GET_DOUBLE](define_group__ConfigMacros_1ga0147d53b34eb0335a4a6385abc13a5ca.md)
  * [Define Documentation](define_group__ConfigMacros_1ga0147d53b34eb0335a4a6385abc13a5ca.md#define-documentation)
    * [`CONFIG_GET_DOUBLE`](define_group__ConfigMacros_1ga0147d53b34eb0335a4a6385abc13a5ca.md#c.CONFIG_GET_DOUBLE)

* [Define CONFIG_GET_INT](define_group__ConfigMacros_1gadf01ea7dbf126644817dd5c05e05b9e3.md)
  * [Define Documentation](define_group__ConfigMacros_1gadf01ea7dbf126644817dd5c05e05b9e3.md#define-documentation)
    * [`CONFIG_GET_INT`](define_group__ConfigMacros_1gadf01ea7dbf126644817dd5c05e05b9e3.md#c.CONFIG_GET_INT)

* [Define CONFIG_GET_STRING](define_group__ConfigMacros_1gaa2f220298348132a67571dad5507c26c.md)
  * [Define Documentation](define_group__ConfigMacros_1gaa2f220298348132a67571dad5507c26c.md#define-documentation)
    * [`CONFIG_GET_STRING`](define_group__ConfigMacros_1gaa2f220298348132a67571dad5507c26c.md#c.CONFIG_GET_STRING)

* [Define CONFIG_GET_UINT](define_group__ConfigMacros_1ga8ea6856050a3db280ede6c6beef01a9a.md)
  * [Define Documentation](define_group__ConfigMacros_1ga8ea6856050a3db280ede6c6beef01a9a.md#define-documentation)
    * [`CONFIG_GET_UINT`](define_group__ConfigMacros_1ga8ea6856050a3db280ede6c6beef01a9a.md#c.CONFIG_GET_UINT)

* [Define GAUSSIAN_EXTRACTOR_AUTHOR](define_group__AuthorInfo_1ga3eb846610f76bc8a47ff45bc10f45696.md)
  * [Define Documentation](define_group__AuthorInfo_1ga3eb846610f76bc8a47ff45bc10f45696.md#define-documentation)
    * [`GAUSSIAN_EXTRACTOR_AUTHOR`](define_group__AuthorInfo_1ga3eb846610f76bc8a47ff45bc10f45696.md#c.GAUSSIAN_EXTRACTOR_AUTHOR)

* [Define GAUSSIAN_EXTRACTOR_COPYRIGHT](define_group__AuthorInfo_1ga0d6d426b032f450f0ac4828d142670a1.md)
  * [Define Documentation](define_group__AuthorInfo_1ga0d6d426b032f450f0ac4828d142670a1.md#define-documentation)
    * [`GAUSSIAN_EXTRACTOR_COPYRIGHT`](define_group__AuthorInfo_1ga0d6d426b032f450f0ac4828d142670a1.md#c.GAUSSIAN_EXTRACTOR_COPYRIGHT)

* [Define GAUSSIAN_EXTRACTOR_DESCRIPTION](define_group__BuildInfo_1gacd8b749a94b6a3362f4329aa020d39cc.md)
  * [Define Documentation](define_group__BuildInfo_1gacd8b749a94b6a3362f4329aa020d39cc.md#define-documentation)
    * [`GAUSSIAN_EXTRACTOR_DESCRIPTION`](define_group__BuildInfo_1gacd8b749a94b6a3362f4329aa020d39cc.md#c.GAUSSIAN_EXTRACTOR_DESCRIPTION)

* [Define GAUSSIAN_EXTRACTOR_NAME](define_group__BuildInfo_1gae0dc034eacb81462c564ca52f86728e3.md)
  * [Define Documentation](define_group__BuildInfo_1gae0dc034eacb81462c564ca52f86728e3.md#define-documentation)
    * [`GAUSSIAN_EXTRACTOR_NAME`](define_group__BuildInfo_1gae0dc034eacb81462c564ca52f86728e3.md#c.GAUSSIAN_EXTRACTOR_NAME)

* [Define GAUSSIAN_EXTRACTOR_REPOSITORY](define_group__AuthorInfo_1ga3e32308b3b7392c05f939618fd9722a8.md)
  * [Define Documentation](define_group__AuthorInfo_1ga3e32308b3b7392c05f939618fd9722a8.md#define-documentation)
    * [`GAUSSIAN_EXTRACTOR_REPOSITORY`](define_group__AuthorInfo_1ga3e32308b3b7392c05f939618fd9722a8.md#c.GAUSSIAN_EXTRACTOR_REPOSITORY)

* [Define GAUSSIAN_EXTRACTOR_VERSION_MAJOR](define_group__VersionConstants_1ga9f86c4acdbf0ca0cc4996915da5d2426.md)
  * [Define Documentation](define_group__VersionConstants_1ga9f86c4acdbf0ca0cc4996915da5d2426.md#define-documentation)
    * [`GAUSSIAN_EXTRACTOR_VERSION_MAJOR`](define_group__VersionConstants_1ga9f86c4acdbf0ca0cc4996915da5d2426.md#c.GAUSSIAN_EXTRACTOR_VERSION_MAJOR)

* [Define GAUSSIAN_EXTRACTOR_VERSION_MINOR](define_group__VersionConstants_1gab8f64056b9a878770492735f1b50e68e.md)
  * [Define Documentation](define_group__VersionConstants_1gab8f64056b9a878770492735f1b50e68e.md#define-documentation)
    * [`GAUSSIAN_EXTRACTOR_VERSION_MINOR`](define_group__VersionConstants_1gab8f64056b9a878770492735f1b50e68e.md#c.GAUSSIAN_EXTRACTOR_VERSION_MINOR)

* [Define GAUSSIAN_EXTRACTOR_VERSION_PATCH](define_group__VersionConstants_1ga1f42fd5602ac563c00f75e18e682fe73.md)
  * [Define Documentation](define_group__VersionConstants_1ga1f42fd5602ac563c00f75e18e682fe73.md#define-documentation)
    * [`GAUSSIAN_EXTRACTOR_VERSION_PATCH`](define_group__VersionConstants_1ga1f42fd5602ac563c00f75e18e682fe73.md#c.GAUSSIAN_EXTRACTOR_VERSION_PATCH)

* [Define GAUSSIAN_EXTRACTOR_VERSION_STRING](define_group__VersionConstants_1ga841d6baf8ac26b79af6561620b8bb8bb.md)
  * [Define Documentation](define_group__VersionConstants_1ga841d6baf8ac26b79af6561620b8bb8bb.md#define-documentation)
    * [`GAUSSIAN_EXTRACTOR_VERSION_STRING`](define_group__VersionConstants_1ga841d6baf8ac26b79af6561620b8bb8bb.md#c.GAUSSIAN_EXTRACTOR_VERSION_STRING)

* [Define HAS_EXECUTION_POLICIES](define_high__level__energy_8h_1af6e9a8d1e68fa55031052926f2e3be85.md)
  * [Define Documentation](define_high__level__energy_8h_1af6e9a8d1e68fa55031052926f2e3be85.md#define-documentation)
    * [`HAS_EXECUTION_POLICIES`](define_high__level__energy_8h_1af6e9a8d1e68fa55031052926f2e3be85.md#c.HAS_EXECUTION_POLICIES)
